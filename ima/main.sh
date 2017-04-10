#!/bin/bash -e

set -x

FILE=/ima/foo
user=71452
IMA_HASH_ALGO=sha256

MODE=$(cat /proc/cmdline | tr " " "\n" | grep ima_appraise)

if [ "$(id -u)" -ne 0 ]; then
	echo "run as root"
fi

if [ "$(readlink /proc/self/ns/mnt)" = "$(readlink /proc/1/ns/mnt)" ]; then
	echo "run in unshare first, i.e. sudo unshare -m $0" && false
fi

mkdir -p $(dirname $FILE)
echo "hello" > $FILE
echo "inode: $(stat -c %i $FILE)"
chown 71452 $FILE

function cleanup()
{
	# rm -rf $(dirname $FILE)
	echo
}

trap cleanup EXIT HUP INT TERM

echo "using IMA hash algo $IMA_HASH_ALGO in $MODE mode"

nsid=$(readlink /proc/self/ns/mnt | cut -c '6-15')
echo "enabling ima for $nsid"
echo "$nsid" > /sys/kernel/security/ima/namespaces
echo my pid: $$

printf "appraise uid=$user\nappraise func=MODULE_CHECK" > "/sys/kernel/security/ima/$nsid/policy"
# printf "appraise fowner=$user\nappraise func=MODULE_CHECK" > "/sys/kernel/security/ima/$nsid/policy"
# printf "measure uid=$user" > "/sys/kernel/security/ima/$nsid/policy"
# printf "measure" > "/sys/kernel/security/ima/$nsid/policy"

function get_file_hash()
{
	hash=$(openssl dgst -$IMA_HASH_ALGO -binary $FILE)
	case $IMA_HASH_ALGO in
		md4 | md5 | sha1)
			echo -e "\x1$hash"
			;;
		sha256)
			echo -e "\x4$hash"
			;;
		*)
			echo "unknown hash $IMA_HASH_ALGO, see <linux/hash_info.h> for offsets" && false
			;;
	esac
}

echo "hash is: $(openssl dgst -$IMA_HASH_ALGO $FILE)"
setfattr -n security.ima -v "$(get_file_hash $FILE)" $FILE

cat "/sys/kernel/security/ima/$nsid/policy"

cat /sys/kernel/security/ima/ascii_runtime_measurements
echo "goodbye" > $FILE
setuid $user strace unshare -r cat $FILE
echo file access succeeded?
cat /sys/kernel/security/ima/ascii_runtime_measurements
