import fcntl
import socket
import struct

ID_TABLE = {
    'dc:a6:32:00:58:29': 1001,
    'dc:a6:32:00:58:32': 1002,
    'dc:a6:32:00:58:2f': 1003
}


class IDFinder:
    def __init__(self, ifname='eth0'):
        self.mac_addr = self._get_hw_addr(ifname)

    def _get_hw_addr(self, ifname):
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        try:
            info = fcntl.ioctl(s.fileno(), 0x8927, struct.pack('256s', bytes(ifname, 'utf-8')[:15]))
            return ':'.join('%02x' % b for b in info[18:24])
        except OSError:
            return '00:00:00:00:00:00'

    def get_id(self):
        if self.mac_addr in ID_TABLE:
            return ID_TABLE[self.mac_addr]
        else:
            return 9999


if __name__ == "__main__":
    id_finder = IDFinder('wlo1')
    print(id_finder.get_id())
