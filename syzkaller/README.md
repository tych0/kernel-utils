to make a syzkaller image:

vmcreate syzkaller
edit /root/.ssh/authorized_keys to allow root logins
change /etc/netplan/50-cloud-init.conf to have matches: {} instead of match on
macaddr
