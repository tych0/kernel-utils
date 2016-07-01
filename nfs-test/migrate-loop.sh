#!/bin/bash

# args 1 & 2 are juju machine IDs that have lxd set up and a container running on 1.

hostA=host-$1
hostB=host-$2

set -e

container=container

failures=0
running_after_dump_failure=0
successes=0

function do_cr() {

  if [ "$(lxc list $hostA:$container | grep -c "\<$container\>")" -eq "0" ]; then
    source=$hostB
    dest=$hostA
    sourcelogs=/var/lib/lxd/logs/$container
    destlogs=/var/lib/lxd2/logs/$container
  else
    source=$hostA
    dest=$hostB
    sourcelogs=/var/lib/lxd2/logs/$container
    destlogs=/var/lib/lxd/logs/$container
  fi

  if [ "$(lxc list $source: | grep "\<$container\>" | grep -ci RUNNING)" -eq "0" ]; then
      echo "starting $source:$container"
      lxc start $source:$container
      sleeptime=$(($(rand -N 1 -M 4)+1))
      echo "waiting $sleeptime seconds"
      sleep $sleeptime
      lxc exec $source:$container systemctl start load
  fi

  bad=0
  echo "moving $source:$container to $dest:$container"
  lxc move $source:$container $dest:$container > /tmp/out || bad=1
  if [ "${bad}" -eq 1 ]; then
    # dumplog="${sourcelogs}/$(sudo ls -1 ${sourcelogs} | grep dump | tail -n1)"
    # restorelog="${destlogs}/$(sudo ls -1 ${destlogs} | grep restore | tail -n1)"

    # # figure out which failed
    # if [ "$(sudo grep -c Error ${dumplog})" -gt "0" ]; then
    #   if [ "$(lxc list $source: | grep "\<$container\>" | grep -ci RUNNING)" -eq "1" ]; then
    #     running_after_dump_failure=$(($running_after_dump_failure+1))
    #   fi

    #   sudo cp $dumplog failurelogs/
    # else
    #   sudo cp $restorelog failurelogs/
    # fi

    failures=$(($failures+1))
    result="failure"
  else
    successes=$(($successes+1))
    result="success"
  fi

  num=$RANDOM
  let "num %= 2"
  if [ "$num" -eq 1 ]; then
      echo "stopping containers"
      lxc stop $source:$container --force &> /dev/null || true
      lxc stop $dest:$container --force &> /dev/null || true
  fi
}

mkdir -p failurelogs

while true; do
  echo "=========> starting c/r cycle"
  do_cr || true
  echo "=========> finished c/r: " ${result}
  echo "successes:" ${successes} "failures:" ${failures} "running after dump failure:" ${running_after_dump_failures}
done
