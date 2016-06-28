#!/bin/bash

set -ex

juju ssh $2 lxc delete --force container || true
juju ssh $3 lxc delete --force container || true
