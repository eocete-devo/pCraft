from scapy.all import Ether, IP, TCP
from . import _utils as utils
import random
import os
import time

class PCraftPlugin(object):
    name = "HTTPPostConnection"
    
    def __init__(self, plugins_data):
        self.plugins_data = plugins_data
        self.random_client_ip = utils.getRandomIP("192.168.0.0/16", ipfail="172.16.42.42")

    def run(self, script=None):
        try:
            srcip = self.plugins_data._get("srcip")
        except KeyError:
            self.plugins_data._set("srcip", self.random_client_ip.get())
        # print("Create a HTTP connection from %s to %s" % (inobj["srcip"],inobj["domain"]))

        srcport = random.randint(4096,65534)
        last_ack = utils.append_tcp_three_way_handshake(self.plugins_data, srcport)

        httpget_string = "POST /g.php HTTP/1.1\r\nAccept: */*\r\nUser-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; InfoPath.1)\r\nHost:" + self.plugins_data._get("domain") + "\r\nContent-Length:122\r\nConnection: Keep-Alive\r\nCache-Control: no-cache\r\n\r\n" + str(os.urandom(122))
        httpreq1 = Ether() / IP(src=self.plugins_data._get("srcip"),dst=self.plugins_data._get("dstip")) / TCP(sport=srcport,dport=80, seq=last_ack[TCP].ack, ack=last_ack[TCP].seq, flags="P""A") / httpget_string
        self.plugins_data.pcap.append(httpreq1)

        # Add the ACK To last http frame
        # "Wed, 09 May 2012 21:35:50 GMT"
        datestr = time.strftime("%a, %d %b %Y %H:%M:%S %Z",time.gmtime())
        httpget_string = "HTTP/1.1 200 OK\r\nServer: nginx\r\bDate: " + datestr + "\r\nContent-Type: text/html\r\nTransfer-Encoding: chunked\r\nConnection: keep-alive\r\nX-Powered-By: PHP/5.3.11-1~dotde b\r0\r\n\r\n" 
        httpreq2 = Ether() / IP(src=self.plugins_data._get("dstip"),dst=self.plugins_data._get("srcip")) / TCP(sport=80,dport=srcport, seq=httpreq1[TCP].ack, ack=httpreq1[TCP].seq + 1, flags="P""A") / httpget_string
        self.plugins_data.pcap.append(httpreq2)
        
        
        return script["_next"], self.plugins_data
