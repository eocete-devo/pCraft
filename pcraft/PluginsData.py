import time
from scapy.utils import PcapWriter

class PluginsData(object):
    def __init__(self, ami, pcapout):
        self.pcap = []
        self.args = {}
        self.current_time = time.time()
        self.ami = ami
        if self.ami.GetStartTime() > 0:            
            self.packet_time = self.ami.GetStartTime()
        else:
            self.packet_time = self.current_time - self.ami.GetSleepCursor()

        self.outpcap = PcapWriter(pcapout, append=True, sync=True)
            
    def AddPacket(self, action, pkt):
        pkt.time = self.packet_time + action.GetSleepCursor()
        self.outpcap.write(pkt)
#        self.pcap.append(pkt)
        
    def _set(self, key, value):
        self.args[key] = value

    def _get(self, key):
        return self.args[key]

    def __str__(self):
        return str(self.args)
        
