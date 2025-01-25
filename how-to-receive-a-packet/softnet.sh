#!/bin/bash

cmd="${0##*/}"

usage() {
cat >&2 <<EOI
usage: $cmd [ -h ]

Output column definitions:
      cpu  # of the cpu 

    total  # of packets (not including netpoll) received by the interrupt handler
             There might be some double counting going on:
                net/core/dev.c:1643: __get_cpu_var(netdev_rx_stat).total++;
                net/core/dev.c:1836: __get_cpu_var(netdev_rx_stat).total++;
             I think the intention was that these were originally on separate
             receive paths ... 

  dropped  # of packets that were dropped because netdev_max_backlog was exceeded

 squeezed  # of times ksoftirq ran out of netdev_budget or time slice with work
             remaining

collision  # of times that two cpus collided trying to get the device queue lock.

EOI
    exit 1
}

if [ "$1" = "-h" ]; then
    usage
fi

softnet_stats_header() {
    printf "%3s %10s %10s %10s %10s %10s %10s\n" cpu total dropped squeezed collision rps flow_limit
}

softnet_stats_format() {
    local cpu=$1
    local total_hex=$2
    local dropped_hex=$3
    local squeezed_hex=$4
    local collision_hex=$5
    local rps_hex=$6
    local flow_limit_hex=${7:-0}

    local total_dec=$((16#$total_hex))
    local dropped_dec=$((16#$dropped_hex))
    local squeezed_dec=$((16#$squeezed_hex))
    local collision_dec=$((16#$collision_hex))
    local rps_dec=$((16#$rps_hex))
    local flow_limit_dec=$((16#$flow_limit_hex))
    
    printf "%3u %10lu %10lu %10lu %10lu %10lu %10lu\n" "$cpu" "$total_dec" "$dropped_dec" "$squeezed_dec" "$collision_dec" "$rps_dec" "$flow_limit_dec"
}

is_lxc_container() {
    if systemd-detect-virt -c | grep -qi "lxc"; then
        return 0
    fi
    return 1
}

parse_cpu_ranges() {
    local IFS=','
    local range cpu_ranges=$1
    local cpus=()

    for range in $cpu_ranges; do
        if [[ $range =~ - ]]; then
            IFS='-' read start end <<< "$range"
            for ((i=start; i<=end; i++)); do
                cpus+=($i)
            done
        else
            cpus+=($range)
        fi
    done
  echo "${cpus[@]}"
}

get_lxc_cpus() {
    local cpus
    if [ -f "/sys/fs/cgroup/cpuset/lxc/cpuset.cpus" ]; then
        cpus=$(cat "/sys/fs/cgroup/cpuset/lxc/cpuset.cpus")
    elif [ -f "/sys/fs/cgroup/cpuset/cpuset.cpus" ]; then
        cpus=$(cat "/sys/fs/cgroup/cpuset/cpuset.cpus")
    else
        echo "Unable to determine allocated CPUs for LXC"
        exit 1
    fi
    parse_cpu_ranges "$cpus"
}

process_softnet_line() {
    local line=$1
    local cpu_index=$2

    set -- $line
    local total=$1
    local dropped=$2
    local squeezed=$3
    local j1=$4
    local j2=$5
    local j3=$6
    local j4=$7
    local j5=$8
    local collision=$9
    local rps=${10}
    local flow_limit_count=${11:-0}

    softnet_stats_format "$cpu_index" "$total" "$dropped" "$squeezed" "$collision" "$rps" "$flow_limit_count"
}

cpu=0
# for normal header
if is_lxc_container; then
    echo "Running inside LXC container."
    cpus=($(get_lxc_cpus))  # Получаем массив CPU
    echo "CPUs allocated to this container: ${cpus[@]}"
fi
softnet_stats_header

if is_lxc_container; then
    cpus=($(get_lxc_cpus))  # Получаем массив CPU
    while read line
    do
        if [[ " ${cpus[*]} " =~ " $cpu " ]]; then
            process_softnet_line "$line" "$cpu"
        fi
        ((cpu++))
    done < /proc/net/softnet_stat
else
#    echo "Not running inside LXC container."
    while read line
    do
        process_softnet_line "$line" "$cpu"
        ((cpu++))
    done < /proc/net/softnet_stat
fi
