BEGIN {
	in_shdr = 0;
	num_sects = 0;
}

/Section Headers:/ {
	in_shdr = 1;
	next;
}

/Key to Flags:/ {
	in_shdr = 0;
	next;
}

function add_section(_name, _addr, _offs, _size)
{
	sects[num_sects]["name"] = _name;
	sects[num_sects]["addr"] = strtonum("0x" _addr);
	sects[num_sects]["offs"] = strtonum("0x" _offs);
	sects[num_sects]["size"] = strtonum("0x" _size);
	num_sects++;
}

{
	if (in_shdr) {
		if ($1 == "[")
			name_offs = 3;
		else
			name_offs = 2;

		name = $(name_offs);
		addr = $(name_offs + 2);
		offs = $(name_offs + 3);
		size = $(name_offs + 4);
		flags = $(name_offs + 6);

		if (flags == "AX" || flags == "WA") {
			add_section(name, addr, offs, size);
		}
	}
}

function print_sect(_name, _addr, _size, _round_up, _print_num_pages,
		#local variables
		_size_kib, _num_pages)
{
	if (_size == 0) {
		_size_kib = 0;
		_num_pages = 0;
	} else {
		if (_round_up) {
			_size_kib = (_size - 1) / 1024 + 1;
		} else {
			_size_kib = _size / 1024;
		}
		_num_pages = (_size - 1) / 4096 + 1;
	}

	

	printf "%-16s", _name;
	printf " %.8X - %.8X", _addr, _addr + _size;
	printf " size %.8X %3d KiB", _size, _size_kib

	if (_print_num_pages)
		printf " %d pages", _num_pages;
	printf "\n";
}

END {
	for (i = 0; i < num_sects; i++ ) {
		addr = sects[i]["addr"];

		if (addr != 0) {
			first_addr = addr;
			break;
		}
	}

	last_addr = sects[num_sects - 1]["addr"];
	last_size = sects[num_sects - 1]["size"];
	ram_usage = last_addr + last_size - first_addr;
	print_sect("RAM Usage", first_addr, ram_usage, 1, 1);

	last_addr = 0;
	last_size = 0;

	for (i = 0; i < num_sects; i++ ) {
		name = sects[i]["name"];
		addr = sects[i]["addr"];
		size = sects[i]["size"];

		if (last_addr != 0 && addr != last_addr + last_size)
			print_sect("*hole*", last_addr + last_size,
					addr - (last_addr + last_size), 0, 0);
		print_sect(name, addr, size, 0, 0);
		last_addr = addr;
		last_size = size;
	}
}
