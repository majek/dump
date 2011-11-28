# Minimal list of escaped characters:
# \ " \x00-\x1f \ud800\udfff \uffff
# 1 1    32     2048         1
# in total: 2083 escaped chars


u_hex = (char_code) ->
    '\\u' + ('0000' + char_code.toString(16)).slice(-4)

run = (r, progress, done) ->
    bad = []
    bad_count = 0
    bad_range = [-1, -1]
    mark_bad = (char_code) ->
        bad_count += 1
        if bad_range[1] + 1 is char_code
            bad_range[1] = char_code
        else
            flush_bad()
            bad_range[0] = char_code
            bad_range[1] = char_code

    flush_bad = () ->
        if bad_range[0] is -1 and bad_range[1] is -1
            return
        if bad_range[0] is bad_range[1]
            bad.push(u_hex(bad_range[0]))
        else
            bad.push(u_hex(bad_range[0]) + '-' + u_hex(bad_range[1]))
        bad_range = [-1, -1]


    arr_chars = for i in [0..65536]
                    # Skipping surrogates for a while.
                    if i >= 0xD800 and i <= 0xDFFF
                        continue
                    String.fromCharCode(i)
    chars = arr_chars.join('')
    r.send(chars)
    progress(u_hex(chars.charCodeAt(0)))

    r.onmessage = (e) ->
        msg2 = e.data
        min =  if msg2.length < chars.length then msg2.length else chars.length
        lim = chars.length

        if chars.substr(0, min) is msg2
            chars = chars.substr(min)
        else
            for i in [0...min]
                if chars.charCodeAt(i) isnt msg2.charCodeAt(i)
                    mark_bad(chars.charCodeAt(i))
                    chars = chars.substr(i+1)
                    lim = 64
                    break
        if chars.length > 0
            r.send(chars.substr(0, lim))
            progress(u_hex(chars.charCodeAt(0)))
        else
            flush_bad()
            bad.sort()
            done(bad, bad_count)