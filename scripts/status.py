import sys


if len(sys.argv) != 2:
	exit(0)

guiID = int(sys.argv[1])
comp = -1
for line in open("computers.txt"):
	if len(line.rstrip()) == 0:
		continue

	rC, gC, group = map(int, line.split())
	if gC == guiID:
		comp = rC
		break

if comp == -1:
	exit(0)

exit(1)

