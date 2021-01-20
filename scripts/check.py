import sys

res = 0
if len(sys.argv) != 3:
	res = 0
	with open('../logs/logs.txt', 'a') as f:
		f.write('Incorrect format\n')
else:
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
		res = 0
	else:
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
	with open('../logs/logs.txt', 'a') as f:
		if res == 1:
			f.write('Got match with [' + str(comp) + ', ' + str(pswd) + ']\n')
		else:
			f.write('Did not get match with [' + str(comp) + ', ' +  str(pswd) + ']\n')
exit(res)

