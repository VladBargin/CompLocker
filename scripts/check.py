import sys


if len(sys.argv) != 3:
	exit(0)

guiID = int(sys.argv[1])
pswd = sys.argv[2].rstrip()


comp = -1

for line in open("computers.txt"):
	try:
		rC, gC, group =  map(int, line.split())
		if gC == guiID:
			comp = rC
			break
	except:
		pass



if comp == -1:
	exit(0)

res = 0
for line in open("pincodes.txt"):
	try:
		ar = line.split()
		if int(ar[0]) == comp:
			for psw in ar[1:]:
				print psw, pswd
				if psw.rstrip() == pswd:
					print "OK"
					res = 1
	except:
		pass

exit(res)

