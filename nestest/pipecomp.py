with open("logs/nestest.log") as f:
	log = [l for l in f.read().split("\n") if l]

i = -1
error = 0

prev2 = ["line -3", "line -2", "line -1"]

try:
	for line in log:

		addr = line[:4].lower()
		op = line[6:14].lower()
		reg = line[48:62].lower()
		p = f"p:{int(line[65:67], 16):08b}"
		sp = line[68:73].lower()
		ppu = line[74:85].lower() # ignore for now
		cycles = line[86:].lower()

		line = f"{addr} {op[:2]} {reg} {p} {sp} {cycles}"

		i += 1
		prev2.pop(0)
		cpu_line = input()
		while not cpu_line.startswith("cpu: "):
			print(f"make: {cpu_line}")
			cpu_line = input()
		prev2.append(line)
		if "cpu: " + line == cpu_line:
			# print(f"{i:04d} {line}")
			continue
		print("mismatch in log lines. printing 3 leading + mismatched emu line")
		print(f"l-2: {prev2[0]}")
		print(f"l-1: {prev2[1]}")
		print(f"log: {line}")
		print(cpu_line)
		error += 1
		if error == 10:
			break
except EOFError:
	print("-- emulator program ended")
