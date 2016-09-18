import socket
import sys

def get_gps():
	# Create a TCP/IP socket
	s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
	#s.connect(('gmail.com', 0))
	IP = socket.gethostbyname(socket.gethostname())
	# Bind the socket to the port

    server_address = (str(IP), 10000)
    print >>sys.stderr, 'connecting to %s port %s' % server_address
    s.connect(server_address)
    
    data = None
	try:
	    
	    # Send data
	    message = 1
	    print >>sys.stderr, 'sending "%s"' % message
	    sock.sendall(message)

	    # Look for the response
	    amount_received = 0
	    amount_expected = 2
	    
	    while amount_received < amount_expected:
	        data = sock.recv(16)
	        amount_received += len(data)
	        print >>sys.stderr, 'received "%s"' % data

	finally:
		return data
	    print >>sys.stderr, 'closing socket'
	    sock.close()
