import sys


if len(sys.argv) != 2:
	exit(0)

guiID = int(sys.argv[1])
group = -1
for line in open("computers.txt"):
	if len(line.rstrip()) == 0:
		continue

	rC, gC, gp = map(int, line.split())
	if gC == guiID:
		group = gp
		break

if group != -1:
	print "Closed lock of group:", group

