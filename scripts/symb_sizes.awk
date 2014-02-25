BEGIN {
	in_symtab = 0;
	num_syms = 0;
}

/Symbol table/ {
	in_symtab = 1;
}

/^$/ {
	in_symtab = 0;
}

function add_symbol(_name, _type, _size, _addr)
{
	syms[num_syms]["name"] = _name;
	syms[num_syms]["type"] = _type;
	syms[num_syms]["size"] = _size;
	syms[num_syms]["addr"] = _addr;
	num_syms++;
}

/^[[:blank:]]*[[:digit:]]*:/ {
	if (in_symtab) {
		size = strtonum($3);

		if (size > 0) {
			addr = strtonum("0x" $2);
			type = $4;
			name = $8;

			add_symbol(name, type, size, addr);
		}
	}
}

function cmp_sym(_i1, _v1, _i2, _v2)
{
	return _v2["size"] - _v1["size"];
}

END {
	printf "%-32s %-6s %16s %8s\n", "Name", "Type", "Address", "Size";

	PROCINFO["sorted_in"] = "cmp_sym";
	for (i in syms) {
		name = syms[i]["name"];
		type = syms[i]["type"];
		addr = syms[i]["addr"];
		size = syms[i]["size"];

		printf "%-32s %-6s", name, type;
		if (addr <= 0xffffffff)
			printf " %8s%08X", "", addr;
		else
			printf " %016X", addr;
		printf " %8X (%5d bytes)\n", size, size;
	}
}
