#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/dlm.h>
#include <linux/dlm_device.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

int main(void)
{
	int ctrl, fd, ret;
	char buf[sizeof(struct dlm_write_request) + 10];
	struct dlm_write_request *req = (void *)buf;
	struct sockaddr_storage storage;
	struct sockaddr_in *addr = (void *)&storage;

	addr->sin_family = AF_INET;
	addr->sin_port = 5000;
	inet_pton(AF_INET, "127.0.0.1", &addr->sin_addr);

	if (mkdir("/sys/kernel/config/dlm/foo", 700) && errno != EEXIST) {
		perror("make dlm foo");
		return 1;
	}

	if (mkdir("/sys/kernel/config/dlm/foo/comms/foo", 700) && errno != EEXIST) {
		perror("make comm foo");
		return 1;
	}

	fd = open("/sys/kernel/config/dlm/foo/comms/foo/local", O_WRONLY);
	if (fd < 0) {
		perror("open local");
		return 1;
	}

	ret = write(fd, "1", 1);
	close(fd);
	if (ret < 0) {
		perror("write()");
		return 1;
	}

	fd = open("/sys/kernel/config/dlm/foo/comms/foo/addr", O_WRONLY);
	if (fd < 0) {
		perror("open addr");
		return 1;
	}

	ret = write(fd, &storage, sizeof(storage));
	close(fd);
	if (ret < 0) {
		perror("write()");
		return 1;
	}

	ctrl = open("/dev/dlm-control", O_RDWR);
	if (ctrl < 0) {
		perror("open ctrl");
		return 1;
	}

	req->version[0] = DLM_DEVICE_VERSION_MAJOR;
	req->version[1] = DLM_DEVICE_VERSION_MINOR;
	req->version[3] = DLM_DEVICE_VERSION_PATCH;
	req->cmd = DLM_USER_CREATE_LOCKSPACE;
	req->is64bit = 1;
	req->i.lspace.flags = 0;
	req->i.lspace.minor = 0;
	//strcpy(req->i.lspace.name, "foo");
	if (write(ctrl, buf, sizeof(struct dlm_write_request) + 4) < 0) {
		perror("write request");
		return 1;
	}

	return 0;
}
