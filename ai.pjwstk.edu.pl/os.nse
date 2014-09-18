id = "os"
description = ""
author = "Marek Majkowski <majek04+nse@gmail.com>"
license = "See nmaps COPYING for licence"

require "bit"
require "npacket"
require "listop" -- filter
require "ipOps"
PQueue = require "loop.collection.PriorityQueue"

--[[
    Remote operating system detection based on Nmaps second gen OS engine.
    But it probes opened ports instead of hosts.

1. Send packets P1-P6 with delay 110ms.
2. Send ECN,T2,T3,T4 (timing based on rtt)
3. Produce results.


Project.
1. Send packets
2. Produce response
3. Parse and compare file
4. RTT

]]--


---------------------------------------------------------
-- UTILS
-- join values from many dicts into one.
-- values from the last table are on top
function join_dicts(...)
    local dd = {}
    for i,v in ipairs(arg) do
	    for key, val in pairs(v) do
	        dd[key] = val
	    end
    end
    return dd
end

-- trim/strip spaces from string
function trim (s)
    return (string.gsub(s, "^%s*(.-)%s*$", "%1"))
end
---------------------------------------------------------
-- PORTRULE
-- For every open tcp port
portrule = function(host, port)
        if port.protocol == "tcp" and port.state == "open" and host.interface then
                return true
        end
        return false
end


---------------------------------------------------------
-- PCAP CALLBACK, returns hash
-- host_ip_bin .. " " .. seq_group
function packet_hash(ip_src_bin, sport, dport)
    return ip_src_bin .. " ".. sport .. " " .. bit.band(dport, 0xfff8)
end

pcap_callback = function(packetsz, layer2, layer3)
    local packet = npacket.new(layer3)
    if packet.tcp_sport ~= nil then
        return packet_hash(packet:ip_bin_src(), packet:tcp_sport(), packet:tcp_dport() )
    end
    return layer3 -- nothing 
end


function new_port_group(host_ip)
    local port = 0
    while true do
        port = bit.band(npacket.rand16(), 0xfff8)
        local key = string.format("os_port_%s_%i", host_ip, port)
        if nmap.registry[key] == nil then
            nmap.registry[key] = true
            break
        end
    end
    return port
end

-- don't pollute the registry
function del_port_group(host_ip, port)
    port = bit.band(port, 0xfff8)
    local key = string.format("os_port_%s_%i", host_ip, port)
    nmap.registry[key] = nil
end

---------------------------------------------------------
-- 

-- Send and receive packets.
-- packets is list of packets [packet, packet, packet]
-- where every packet mus have:
--    packet.send_time - time in ms since epoch when to send
--    packet.buf       - binary buffer with packet to send
-- as the result this is set for every packet:
--    packet.response  - response packet structure
--    packet.response.buf - binary buffer with response packet
--    packet.retries   - how many times we send this one beforegetting response
function packet_send_loop_by_lport(packets, max_retries, rtt, pcap, dnet, hash, debug)
    local queue = {}
    
    local now = nmap.clock_ms()
    local port_number
    local packet, send_time
    local packets_by_lport  = {} -- port on local side
    for i, packet in ipairs(packets) do
        if not port_number then
            port_number = packet:tcp_dport()
        end
        packets_by_lport[packet:tcp_sport()] = packet
        PQueue.enqueue(queue, packet, packet.send_time)
        packet.retries = 0
    end
    
    local data_to_send = nil
    local print_queue = function (q)
        local now = nmap.clock_ms()
        local t ={}
        for v in PQueue.sequence(q) do
            local k = PQueue.priority(q, v)
            table.insert(t, "#" .. v.number .. "=" .. (k - now) )
        end
        stdnse.print_debug(debug+1, "%i#[%s]", port_number, table.concat(t, " ") )
    end
    
    local variance = 0
    local variance_ct = 0
    -- send packets, and wait for responses
    while not PQueue.empty(queue) do
        print_queue(queue)
        now = nmap.clock_ms()
        -- get packet from queue
        packet, send_time = PQueue.dequeue(queue)
        
        --stdnse.print_debug(0, "%i# TDiff = %ims", packet.number, now-send_time)
        
        -- send packet and enqueue it in the future
        if packet.retries < max_retries then
            packet.retries = packet.retries + 1
            PQueue.enqueue(queue, packet, send_time + rtt)
            stdnse.print_debug(debug+1, "%i#%i sending retr=%i with drift %ims", port_number,packet.number, packet.retries, send_time-now)
            data_to_send = packet.buf
            packet.send_time = now
        else -- discard
            stdnse.print_debug(debug+1, "%i#%i packet discarded after %i retries", port_number,packet.number, packet.retries)
            data_to_send = nil
        end
        
        local var = send_time - now
        variance = variance + var*var
        variance_ct = variance_ct + 1
        

        -- wait for packets or delay
        while true do
            -- after new timeout is enqueued register to pcap
            if PQueue.empty(queue) then
                break
            end
            now = nmap.clock_ms()
            local packet2    = PQueue.head(queue)
            local next_timer = PQueue.priority(queue, packet2)
            local recv_delay = next_timer - now
            if recv_delay <= 2 then -- next event is in the past
                recv_delay = 0
                if not data_to_send then -- no sending? quit!
                    break
                end
            end
            stdnse.print_debug(debug+2, "%i# waiting %ims", port_number, recv_delay)
            pcap:set_timeout(recv_delay)
            pcap:pcap_register(hash)
            if data_to_send then
                dnet.ip_send(data_to_send)
                data_to_send = nil
            end

            local status, _, l2_data, l3_data, ts
            -- block
            status, _, l2_data, l3_data, ts = pcap:pcap_receive()
            local var = nmap.clock_ms()-now
            stdnse.print_debug(debug+2, "%i# waited %ims >= %ims  drift=", port_number, recv_delay,var,var-recv_delay)
             
            now = nmap.clock_ms()
            if status then -- got packet
                local pck = npacket.new(l3_data)
                if   packets_by_lport[pck:tcp_dport()] and
                     --packets_by_lport[pck:tcp_dport()]:tcp_dport() == pck:tcp_sport() and
                     l3_data:len() >= pck.tcp_data_offset then -- packet not too short
                   packet = packets_by_lport[pck:tcp_dport()]
                   packet.response = pck
                   pck.probe = packet
                   if ts ~= nil then
                       pck.recv_time = ts -- if timestamp is got from pcap, than the timing is
                       -- much more accurate. But this needs patch :)
                   else
                       pck.recv_time = now
                   end
                   -- mark that we got response
                   PQueue.remove(queue, packet)
                   stdnse.print_debug(debug+1, "%i#%i got ack", port_number, packet.number)
                end
            end
        end -- time passed
    end -- all packets and responses done
    return math.sqrt(variance / variance_ct)
end

---------------------------------------------------------
-- 

function mod_diff(a,b)
     -- #define MOD_DIFF(a,b) ((u32) (MIN((u32)(a) - (u32 ) (b), (u32 )(b) - (u32) (a))))
     local x = bit.band(a - b, 0xffffffff)
     local y = bit.band(b - a + 0xffffffff,0xffffffff)
     if x < y then return x end
     return y
end
--[[
unsigned int gcd_n_uint(int nvals, unsigned int *val)
 {
   unsigned int a,b,c;
   
   if (!nvals) return 1;
   a=*val;
   for (nvals--;nvals;nvals--)
     {
       b=*++val;
       if (a<b) { c=a; a=b; b=c; }
       while (b) { c=a%b; a=b; b=c; }
     }
   return a;
 }
]]--
function gcd_n_uint(val)
    if #val == 0 then
        return 1
    end
    local a, b, c, f 
    for _,b in ipairs(val) do
        if not f then -- first run
            a = b
            f = true
        else -- second and next runs
            if a<b then
                c = a
                a = b
                b = c
            end
            while b ~= 0 do
                c = math.floor(a % b)
                a = b
                b = c
            end
        end
    end
    return a
end


function get_ipid_sequence(ipids, islocalhost)
    local allipideqz = true
    local f = true
    local prev 
    local ipid_diffs = {}
    for i, ipid in ipairs(ipids) do
        local ipid_diff
        if ipid ~= 0 then allipideqz = false end
        if f then -- first run
            f=false
            -- ignore   
        else -- second
            local a = prev
            local b = ipid
            if a <= b then
                ipid_diff = b - a
            else
                ipid_diff = bit.band(b - a + 0xffff, 0xffff) -- u16
            end
            if ipid_diff > 20000 then
                return "RD"
            end
            table.insert(ipid_diffs, ipid_diff)
        end
        prev = ipid
    end
    
    if allipideqz then
        return "Z"
    end
    if islocalhost then
        local allgto = 1
        for i, ipid_diff in ipairs(ipid_diffs) do
            if ipid_diff < 2 then
                allgto = 0
                break
            end
        end
        if allgto then
            for i, ipid_diff in ipairs(ipid_diffs) do
                if math.floor(ipid_diff % 256) == 0 then
                    ipid_diffs[i] = ipid_diff - 256
                else
                    ipid_diffs[i] = ipid_diff - 1
                end
            end
        end
    end
    
    -- constant finding - broken code, works only for 0
    local x = 0
    for i, ipid_diff in ipairs(ipid_diffs) do
        if ipid_diff ~= x then
            x = nil
            break
        end
    end
    
    if x ~= nil then
        return string.format("%X", x) 
    end
    
    for i, ipid_diff in ipairs(ipid_diffs) do
        if (ipid_diff > 1000 and
            (math.floor(ipid_diff % 256) ~= 0 or
            (math.floor(ipid_diff % 256) == 0 and ipid_diff >= 25600))) then
            return "RI"
        end
    end 
    
    local j = true
    local k = true
    for i, ipid_diff in ipairs(ipid_diffs) do
        if (k and (ipid_diff > 5120 or math.floor(ipid_diff % 256) ~= 0)) then
            k = false
        end
        if j and ipid_diff > 9 then
            j = false
        end
    end
    if k then
        return "BI"
    end
    if j then
        return "I"
    end
    return ""
end

function humanize_seconds(d)
    if d < 60 then
        return string.format("%i seconds", d)
    elseif d < 60*60 then
        return string.format("%i minutes",d / (60))
    elseif d < 60*60*24 then
        return string.format("%3.1f hours",d/ (60*60) )
    elseif d < 60*60*24*30 then
        return string.format("%3.1f days",d/ (60*60*24) )
    end
    return string.format("%3.1f months",d/ (60*60*24*30) )
end

function os2_seq(packets, host)
    local output = {}
    
    local seq_diffs = {}
    local time_usec_diffs  = {}
    local seq_rates = {}
    local ts_diffs = {}
    local seq_avg_rate = 0.0
    local prev = nil
    for _, packet in pairs(packets) do
        if prev and packet then
            local seq_diff = mod_diff(packet:tcp_seq(), prev:tcp_seq())
            table.insert(seq_diffs, seq_diff)

            local time_usec_diff = (packet.probe.send_time - prev.probe.send_time) * 1000.0
            if time_usec_diff < 1 then time_usec_diff = 1 end -- less than one us
            table.insert(time_usec_diffs, time_usec_diff)

            local seq_rate = math.floor(seq_diff * 1000000.0 / time_usec_diff)
            table.insert(seq_rates, seq_rate)
            seq_avg_rate = seq_avg_rate + seq_rate 
            
            local tsval1, tsval2 
            tsval1, _ = prev:tcp_option_timestamp() -- tsval, tsecr
            tsval2, _ = packet:tcp_option_timestamp()
            if tsval1 ~= nil and tsval2 ~= nil then
                local ts_diff = mod_diff(tsval2, tsval1)
                table.insert(ts_diffs, ts_diff)
            end      
        end
        prev = packet
    end
    local seq_gcd=gcd_n_uint(seq_diffs)
    
    seq_avg_rate = seq_avg_rate / #seq_rates
    local seq_rate = seq_avg_rate
    local isr=math.floor((math.log(seq_rate) / math.log(2.0)) * 8 + 0.5);
    local sp = 0
    local div_gcd = 1
    local seq_stddev = 0.0
    if seq_gcd > 9 then
        div_gcd = seq_gcd
    end
    for i in ipairs(seq_rates) do
        local rtmp = seq_rates[i] / div_gcd - seq_avg_rate/div_gcd
        seq_stddev = seq_stddev + rtmp * rtmp
    end
    seq_stddev = seq_stddev / (#seq_rates - 2)
    seq_stddev = math.sqrt(seq_stddev)
    if seq_stddev <= 1 then
        sp = 0
    else
        seq_stddev = math.log(seq_stddev) / math.log(2.0)
        sp = math.floor(seq_stddev * 8 + 0.5)
    end
    table.insert(output, string.format("SP=%X", sp) )
    table.insert(output, string.format("GCD=%X", seq_gcd) )
    table.insert(output, string.format("ISR=%X", isr) )
    
    local ti = ''
    if #packets >= 3 then
        local ipids = {}
		for _, packet in ipairs(packets) do
		    table.insert(ipids, packet:ip_id())
		end	    
        ti = get_ipid_sequence(ipids, ipOps.isLocalhost(host))
    end
    table.insert(output, string.format("TI=%s", ti) )

    local ts = "U"
    local lastboot = 0
    if #ts_diffs == #packets-1 then
        local timepck
        for _, p in ipairs(packets) do
            if p:tcp_option_timestamp() then
                timepck = p
                break
            end
        end
        if not timepck then
            stdnse.print_debug(0, "No packet with time? %i %i", #ts_diffs, #packets )
        end

	    local avg_ts_hz = 0.0
	    for i in ipairs(ts_diffs) do
	        local dhz
	        dhz = ts_diffs[i] / (time_usec_diffs[i] / 1000000.0 )
	        avg_ts_hz = avg_ts_hz + (dhz / #ts_diffs)
	    end
        if avg_ts_hz > 0 and avg_ts_hz < 5.66 then
            ts = "1"
            lastboot = timepck.probe.send_time/1000.0 - timepck:tcp_option_timestamp() / 2
        elseif avg_ts_hz > 70 and avg_ts_hz <= 150 then
            ts = "7"
            lastboot = timepck.probe.send_time/1000.0 - timepck:tcp_option_timestamp() / 100
        elseif avg_ts_hz > 150 and avg_ts_hz <= 350 then
            ts = "8"
        elseif avg_ts_hz > 724 and avg_ts_hz < 1448 then
            lastboot = timepck.probe.send_time/1000.0 - timepck:tcp_option_timestamp() / 1000
        end
        
        if ts == "U" then
            ts = string.format("%X", math.floor(0.5 + math.log(avg_ts_hz)/math.log(2.0)) )
        end
        if lastboot == 0 then
            lastboot = timepck.probe.send_time/1000.0 - 
                                timepck:tcp_option_timestamp() / math.floor(0.5 + avg_ts_hz)
        end
        if lastboot and (timepck.probe.send_time/1000.0 - lastboot > 63072000) then
            -- Up 2 years?  Perhaps, but they're probably lying.
            lastboot = 0
        end
    end
    table.insert(output, string.format("TS=%s", ts) )
    if ts ~= "U" then
        table.insert(output, string.format("uptime=%s", humanize_seconds(nmap.clock_ms()/1000.0-lastboot)) )
    end
    return table.concat(output,"%")
end

---------------------------------------------------------
function os2_tcp_options_to_string(packet)
    local optstring = {}
    for _, opt_dd in ipairs(packet:tcp_options()) do
        if     opt_dd.type == 0 then -- eol
            table.insert(optstring, "L")
        elseif opt_dd.type == 1 then -- nop
            table.insert(optstring, "N")
        elseif opt_dd.type == 2 then -- mss
            table.insert(optstring, "M")
            table.insert(optstring, string.format("%X", npacket.u16(opt_dd.data, 0) ))
        elseif opt_dd.type == 3 then -- ws
            table.insert(optstring, "W")
            table.insert(optstring, string.format("%X", npacket.u8(opt_dd.data, 0) ))
        elseif opt_dd.type == 8 then -- ts
            local tsval = npacket.u32(opt_dd.data, 0)
            local tsecr = npacket.u32(opt_dd.data, 0)
            if tsval == 0 then tsval = 0 else tsval = 1 end
            if tsecr == 0 then tsecr = 0 else tsecr = 1 end
            table.insert(optstring, "T")
            table.insert(optstring, tsval)
            table.insert(optstring, tsecr)
        elseif opt_dd.type == 4 then -- sack
            table.insert(optstring, "S")
        end
    end
    return table.concat(optstring)        
end

function os2_ops(packets)
    local resp = {}
    for _, packet in pairs(packets) do
        -- for tcp options...
        if packet.response ~= nil then
            local x = os2_tcp_options_to_string(packet.response)
            table.insert(resp, string.format("O%i=%s", packet.number, x) )
        end
    end
    return table.concat(resp, "%")
end
---------------------------------------------------------
function os2_win(packets)
    local resp = {}
    for _, packet in pairs(packets) do
        if packet.response ~= nil then
            table.insert(resp, "W"..packet.number .."="..
                        string.format("%X", packet.response:tcp_win() ) )  
        end
    end
    return table.concat(resp, "%")
end

---------------------------------------------------------
function yn(x)
    if x > 0 then
        return "Y"
    end
    return "N"
end

function os2_packet_descripton(packet, probe, name)
    local res = {}
    if packet == nil then
        return "R=N"
    end
    table.insert(res, "R=Y")
    table.insert(res, "DF=" .. yn(packet:ip_df()) )
    local tg = 0
    if packet:ip_ttl() <= 32 then tg=32
    elseif packet:ip_ttl() <= 64 then tg=64
    elseif packet:ip_ttl() <= 128 then tg=128
    else tg = 255 end
    table.insert(res, string.format("TG=%X", tg) )
    if name~="T1" then
        table.insert(res, string.format("W=%X", packet:tcp_win() ) )
    end

    if name=="ECN" then   
        table.insert(res, "O=" .. os2_tcp_options_to_string(packet))
        
        local cc
        if packet:tcp_th_ece() == 1 and packet:tcp_th_cwr() == 0 then
            cc = "Y"
        elseif packet:tcp_th_ece() == 0 and packet:tcp_th_cwr() == 0 then
            cc = "N"
        elseif packet:tcp_th_ece() == 1 and packet:tcp_th_cwr() == 1 then
            cc = "S"
        else
            cc = "O"
        end
        table.insert(res, "CC=" .. cc)
    else --not ecn
        local s = ""
	    if packet:tcp_seq() == 0 then
	        s = "Z"
	    elseif packet:tcp_seq() == probe:tcp_ack() then
	        s = "A"
	    elseif packet:tcp_seq() == probe:tcp_ack() + 1 then --TODO: wrapped counter?
	        s = "A+"
	    else
	        s = "O"
	    end
	    table.insert(res, "S=" .. s)
	    
	    local a = ""
	    if packet:tcp_ack() == 0 then
	        a = "Z"
	    elseif packet:tcp_ack() == probe:tcp_seq() then
	        a = "S"
        elseif packet:tcp_ack() == probe:tcp_seq() + 1 then --TODO: wrapped counter?
	        a = "S+"
	    else
	        a = "O"
	    end
	    table.insert(res, "A=" .. a)
	    
	    local f = ""
	    if packet:tcp_th_fin() > 0 then f = "F" .. f end
	    if packet:tcp_th_syn() > 0 then f = "S" .. f end
	    if packet:tcp_th_rst() > 0 then f = "R" .. f end
	    if packet:tcp_th_psh() > 0 then f = "P" .. f end
	    if packet:tcp_th_ack() > 0 then f = "A" .. f end
	    if packet:tcp_th_urg() > 0 then f = "U" .. f end
	    if packet:tcp_th_ece() > 0 then f = "E" .. f end
	    table.insert(res, "F=" .. f)

        if name~="T1" then
            table.insert(res, "O=" .. os2_tcp_options_to_string(packet))
        end
	    
	    local rd = "0"
	    if packet:tcp_th_rst() > 9 and packet:tcp_payload():len() > 0 then
	        rd = string.format("%08lX", npacket.crc16(packet:tcp_payload() ) )
	    end
	    table.insert(res, "RD=" .. rd)
    end -- end not ecn
       
    local q = ""
    if packet:tcp_re0() > 0 then
        q = q .. "R"
    end
    if packet:tcp_urp() > 0 then
        q = q .. "U"
    end
    table.insert(res, "Q=" .. q)
    
    return table.concat(res, "%")
end
---------------------------------------------------------
-- LOADING DATA
-- line is sthing like T1(A=XX%B=ss)
-- the result is name, {O="asd", B="asd",... }
function unpack_line(line)
    local _, name, inside 
    -- string.find("T1(A=asda%B=asd%C=asd%)", "^([A-Za-z0-9]+)[(]([^()]*)[)]$")
    -- 1       23      T1      A=asda%B=asd%C=asd%
    _, _, name, inside = string.find(line, "^([A-Za-z0-9]+)[(]([^()]*)[)]$")

    -- > return string.gsub("A=asda%B=asd%C=asd", "([A-Z]+)=([^=%%]*)%%", function(x,y) return x .. ":" .. y .. " " end )
    -- A:asda B:asd C=asd      2
    local t = {}
    string.gsub(inside, "([A-Z]+)=([^=%%]*)%%", 
            function(k, v)
                t[k] = v 
            end )
    return name, t
end

-- the result:
-- {Class={{"3Com", "embedded"...}, {...}}, Fingerprint="Linux 2.6", T1={O="asd"}... }
function fingerprint_unpack(lines)
    local fingerprint = {}
    for _,line in ipairs(lines) do
        local fc = line:sub(1,2)
        if     fc == "Fi" then
            local _, s
            _, _, s = string.find(line, "^Fingerprint (.*)$")
            fingerprint["Fingerprint"] = s
        elseif fc == "Cl" then
            local _, c
            _, _, c = string.find(line, "^Class (.*)$")
            c = stdnse.strsplit("|", c)
            listop.map(trim, c)
            if fingerprint["Class"] == nil then
                fingerprint["Class"] = {}
            end
            table.insert(fingerprint["Class"], c)
        elseif fc == "SE" or fc == "OP" or fc == "WI" or fc == "EC" or
               fc == "T1" or fc == "T2" or fc == "T3" or fc == "T4" then
            local name, dd
            name, dd = unpack_line(line) 
            fingerprint[name] = dd
        elseif fc == "Ma" or fc == "U1" or fc == "IE" or
               fc == "T5" or fc == "T6" or fc == "T7" then
            -- nothing
        else
            stdnse.print_debug(0, "Wrong line in nmap-os-db! '%s...'", line )
        end        
    end
    return fingerprint
end

function matches(test, val)
    local f = test:sub(1,1)
    if f == ">" then
        local v = npacket.hextodecimal(test:sub(2))
        val = npacket.hextodecimal(val)
        if val > v then
            return true
        else
            return false
        end
    end
    if f == "<" then
        local v = npacket.hextodecimal(test:sub(2))
        val = npacket.hextodecimal(val)
        if val < v then
            return true
        else
            return false
        end
    end
    local _, a, b
    _, _, a, b = string.find(test,"^([A-F0-9]+)-([A-F0-9]+)$")
    if a ~= nil and b ~= nil then
        a = npacket.hextodecimal(a)
        b = npacket.hextodecimal(b)
        val = npacket.hextodecimal(val)
        if a>= val and b<= val then
            return true
        end
        return false
    end
    
    local tests = stdnse.strsplit("|", test)
    for _, t in ipairs(tests) do
        if t == val then
            return true
        end
    end
    return false
end

function fingerprint_match(fp, tfp, matchpoints)
    local nummatchpoints =0
    local possiblepoints = 0
    for linekey, values in pairs(tfp) do
        for key, val in pairs(values) do
            if fp[linekey] == nil then
                -- stdnse.print_debug(2, "no such row in fp %s ", linekey)
            elseif fp[linekey][key] == nil then
                -- stdnse.print_debug(2, "no such key in row in fp %s %s ", linekey, key)
            else
	            if matches(fp[linekey][key], val) then
	                --stdnse.print_debug(3, "match %s %s == true", fp[linekey][key], val)
	                nummatchpoints = nummatchpoints + matchpoints[linekey][key]
	            else
	                --stdnse.print_debug(3, "match %s %s == false", fp[linekey][key], val)
	            end
	            possiblepoints = possiblepoints + matchpoints[linekey][key]
            end
        end
    end
    return  nummatchpoints, possiblepoints
end

-- at most 10 elements
--  { {m=0.1,f=..fingerprint.. }, {m=0.2,f=finerprint}, {} }
function fingerprints_match(fingerprints, tfingerprint, matchpoints, max_results, interface)
    local t0 = nmap.clock_ms()
    local queue = {}

    -- hack used only to go back to event loop
    local pcap = nmap.new_socket()
    function pcap_callback() return 'xxx' end
    pcap:pcap_open(interface, 64, 0, pcap_callback, 
        "tcp[0] == 0xDE and ((tcp[tcpflags] & tcp-fin) != 0) " )

    
    local id = math.random(10, 99) -- some kind of unique id
    local now = nmap.clock_ms()  
    local x0 = now -- running time (start)
    local x1 = now -- (stop)
    local _
    for i, fingerprint in ipairs(fingerprints) do
        if i % 15 == 0 then
            local now = nmap.clock_ms()
            if now-x0 > 8 then -- more than 8 ms?
	            x1 = now
	            local d0 = now  -- sleeping time
	            -- go back to the event loop for a while
	            pcap:set_timeout(0) -- 0ms
	            pcap:pcap_register('aaa')
	            _, _, _, _, _ = pcap:pcap_receive()
                now = nmap.clock_ms()
	            stdnse.print_debug(2, "%i Gave back processor (used=%ims) (sleept=%ims)", id, x1-x0, now-d0)
                x0 = now
            end
        end
        local nummatchpoints, possiblepoints
        nummatchpoints, possiblepoints = fingerprint_match(fingerprint, tfingerprint, matchpoints)
        local m = 1.0 - (nummatchpoints / possiblepoints)
        if m < 0.20 then  -- at least 80% coverage
	        local t = {}
	        t.m = m
	        t.nummatchpoints = nummatchpoints
	        t.possiblepoints = possiblepoints
	        t.f = fingerprint
	        PQueue.enqueue(queue, t, m)
        end
    end
   
    local x = {}
    for i=1,max_results do
        if PQueue.empty(queue) then
            break
        end

        local t, m
        t, m = PQueue.dequeue(queue)
        table.insert(x, t)
    end
    local t1 = nmap.clock_ms()
    stdnse.print_debug(1, "Fingerprints compared in %ims", t1-t0)
    return x
end

function load_os_db()
    local path = nmap.fetchfile("nmap-os-db")
    if path == nil then
        stdnse.print_debug(0, "Can't find path to 'nmap-os-db'! No signatgures loaded!")
        return {}, {}
    end
    stdnse.print_debug(2, "Loading nmap-os-db from %s", path)
    
    local t0 = nmap.clock_ms()
    
    local f = io.open(path, "r")
    local fingerprints = {}
    local fingerprint = {}
    local matchpoints = {}
    local mode = ''
    while true do
        local line = f:read("*line")
        if line == nil then break end
    
        local firstchar = line:sub(1,1)
        if line:len() > 0 and firstchar ~= "#" and firstchar ~= " " and firstchar ~= "\t" then -- not whitespace or comment
            if firstchar == "M" or firstchar == "F" then    -- MathPoints
                if mode == "M" then
                    matchpoints = fingerprint_unpack(fingerprint)
                else
                    table.insert(fingerprints, fingerprint_unpack(fingerprint) )
                end
                mode = firstchar
                fingerprint={}
            end
            table.insert(fingerprint, line)
        end
    end
    if #fingerprint > 0 then
        table.insert(fingerprints, fingerprint_unpack(fingerprint) )
    end
    local t1 = nmap.clock_ms()
    stdnse.print_debug(1, "Loaded %i entries in %ims", #fingerprints, t1-t0)
    f:close()
    return matchpoints, fingerprints
end

---------------------------------------------------------
-- ACTION
action = function (host, port)
    if nmap.registry["nmap-os-db"] == nil then
        local matchpoints, fingerprints
        matchpoints, fingerprints = load_os_db()
        nmap.registry["nmap-os-db"] =  fingerprints
        nmap.registry["nmap-os-db-m"] = matchpoints
    end
    local fingerprints = nmap.registry["nmap-os-db"]
    local matchpoints  = nmap.registry["nmap-os-db-m"]

    local pcap = nmap.new_socket()
    local dnet = nmap.new_dnet()
    -- syn or ack or rst
    pcap:pcap_open(host.interface, 2048, 0, pcap_callback, 
        "tcp and ("..
            "((tcp[tcpflags] & tcp-syn) != 0) "..
            "or ((tcp[tcpflags] & tcp-ack) != 0)" ..
            "or ((tcp[tcpflags] & tcp-rst) != 0)" ..
            ")")

    local common_pre = {
            ip_bin_src=host.bin_ip_src, 
            ip_bin_dst=host.bin_ip,
            tcp_dport=port.number,
            tcp_th_syn=true,}
    
    local packets_stage_first = {
	    -- P1 
	    { tcp_win=1, tcp_options={
                {window=10}, {nop=true}, {mss=1460}, {timestamp={tsval=0xffffffff, tsecr=0}}, {sack=true} }},
	    -- P2
        { tcp_win=63, tcp_options={
                {mss=1400}, {window=0}, {sack=true}, {timestamp={tsval=0xffffffff, tsecr=0}}, {eol=true} }},
	    -- P3
	    { tcp_win=4, tcp_options={
                {timestamp={tsval=0xffffffff, tsecr=0}}, {nop=true}, {nop=true}, {window=5}, {nop=true}, {mss=640} }},
	    -- P4
	    { tcp_win=4, tcp_options={
                {sack=true}, {timestamp={tsval=0xffffffff, tsecr=0}}, {window=10}, {eol=true} }},
	    -- P5
	    { tcp_win=16, tcp_options={
                {mss=536}, {sack=true}, {timestamp={tsval=0xffffffff, tsecr=0}}, {window=10}, {eol=true} }},
	    -- P6
	    { tcp_win=512, tcp_options={
                {mss=265}, {sack=true}, {timestamp={tsval=0xffffffff, tsecr=0}} }},
    }
    local packets_stage_second = {
	    { name="ECN",
	        tcp_th_syn=true, tcp_th_cwr=true, tcp_th_ece=true, tcp_re0=true, 
	        tcp_urp=0xf7f5, tcp_ack=0, tcp_win=3, tcp_options={
	                    {window=10}, {nop=true}, {mss=1460}, {sack=true}, {nop=true}, {nop=true} }},
	    { name="T2", 
	        tcp_th_syn=false, -- null flags
	        ip_df=true, tcp_win=128, tcp_options={
                        {window=10}, {nop=true}, {mss=265}, {timestamp={tsval=0xffffffff, tsecr=0}}, {sack=true} }},
	    { name="T3", 
	        tcp_th_syn=true, tcp_th_fin=true, tcp_th_urg=true, tcp_th_psh=true, 
	        tcp_win=256, ip_df=false, tcp_options={
                        {window=10}, {nop=true}, {mss=265}, {timestamp={tsval=0xffffffff, tsecr=0}}, {sack=true} }},
	    { name="T4", 
	        tcp_th_syn=false, tcp_th_ack=true, 
	        ip_df=true, tcp_win=1024, tcp_options={
                        {window=10}, {nop=true}, {mss=265}, {timestamp={tsval=0xffffffff, tsecr=0}}, {sack=true} }},
    }

    -------- SEQ
    local gresponses
    local gpackets
    local gvar, gtryno
    local gtries = 0
    -- up to four retransmissions
    for xx=1,4 do
        gtries = gtries + 1 
	    local sport=new_port_group(host.bin_ip)
	    local hash = packet_hash(host.bin_ip, port.number, sport)
	    local now = nmap.clock_ms() + 0 -- 0ms after now
	    local packets = {}
	    for i, pck_dd in ipairs(packets_stage_first) do
	        local common_perpacket = {
	            ip_id=npacket.rand16(), 
	            tcp_seq=npacket.rand32(),
	            tcp_sport=sport+i,
	            tcp_ack=npacket.rand32(),
	            ip_ttl=math.random(37,63),
	        }
	        local packet = npacket.new_tcp_packet_from_dict( join_dicts(common_pre, common_perpacket, pck_dd) )
	        packet:count_all()
	        table.insert(packets, packet)
	        packet.number  = i
	        packet.send_time = now + 100 * (i-1)
	    end
	        
	    --                                     max_retries, rtt ms, pcap, dnet, pcap_hash, debug
	    local var = 
 	         packet_send_loop_by_lport(packets, 1,          600,    pcap, dnet, hash, 1)
	    
	    stdnse.print_debug(2, "SEQ scan for port %i. Variance %fms", port.number, var)
	    
	    local r = {}
	    for i, packet in pairs(packets) do
	        if packet.response ~= nil then
	            table.insert(r, packet.response)
	        end
	    end
        del_port_group(host.bin_ip, sport)
	
	    -- if first run
	    --   or received more packets
	    --   or received full packets and timing is better
	    -- so: the best probe that gets all responses
	    if gresponses == nil or #gresponses < #r or (#gresponses == #packets and gvar < var) then
	       gresponses = r
           gpackets   = packets
           gvar       = var
           gtryno     = xx
        end
        if #gresponses == #packets and gvar < 2.0 then -- variance < 2?
            break
        end
    end
    

    local seq_probe = os2_seq(gresponses, host)
    local ops_probe = os2_ops(gpackets)
    local win_probe = os2_win(gpackets)
    local t1_probe  = ''
    if #gresponses > 0 then
        t1_probe = os2_packet_descripton(gresponses[1], gresponses[1].probe, "T1" )
    end
        
    -------- ECN,T2,T3,T4
    local sport= new_port_group(host.bin_ip)
    local hash = packet_hash(host.bin_ip, port.number, sport)

    local now = nmap.clock_ms() + 0 -- 0ms after now
    local packets = {}
    local i = 0
    for _, pck_dd in pairs(packets_stage_second) do
        i = i + 1
        local common_perpacket = {
            ip_id=npacket.rand16(), 
            tcp_seq=npacket.rand32(), 
            tcp_sport=sport+i,
            tcp_ack=npacket.rand32(),
            ip_ttl=math.random(37,63),
        }
        local packet = npacket.new_tcp_packet_from_dict( join_dicts(common_pre, common_perpacket, pck_dd) )
        packet:count_all()
        table.insert(packets, packet)
        packet.number  = i
        packet.send_time = now + 50 * (i-1) -- send_delay
    end
        
    --                                     max_retries, rtt ms, pcap, dnet, pcap_hash
    packet_send_loop_by_lport(packets, 5,          600,    pcap, dnet, hash, 3)
    
    local output = {}
    for _, packet in pairs(packets) do
        local d = os2_packet_descripton(packet.response, packet, packet:name() )
        table.insert(output, string.format("%s(%s)", packet:name(), d))
    end


    del_port_group(host.bin_ip, sport)
    
    -------- FINAL
    pcap:pcap_close()
    
    table.insert(output, "SEQ(" .. seq_probe .. ")")
    table.insert(output, "OPS(" .. ops_probe .. ")")
    table.insert(output, "WIN(" .. win_probe .. ")")
    table.insert(output, "T1(" .. t1_probe .. ")")
    local fingerprint = fingerprint_unpack(output)
    
    
    
    local matches = fingerprints_match(fingerprints, fingerprint, matchpoints, 3, host.interface)
    local o = {}
    for _, t in ipairs(matches) do
        --[[
        local c = {}
        for _, class in pairs(t.f.Class) do
            table.insert(c, class[1] .. " " ..class[2] .. " " .. class[3])
        end
        local x = table.concat(c, "?")]]--
        local x = t.f.Fingerprint
        local s = string.format("%s (%2.0f%%)",x, (1.0-t.m)*100.0)
        table.insert(o, s)
    end
    if nmap.debugging()>0 then
        local info = string.format("seq info: got packets=%i try=%i/%i variance=%f", #gresponses, gtryno, gtries, gvar)
        return table.concat(o, ", ") .. "\n    " .. info .. "\n    " .. table.concat(output, "\n    ")  
    end
    return table.concat(o, ", ")
end





