# --------------------------------- expr.awk -----------------------------------

#
# This is part of the flight simulator 'fly8'.
# Author: Eyal Lebedinsky (eyal@eyal.emu.id.au).
#

#
# Convert a (cpp processed) object description to a simple list of numbers.
# The input should be a comma separated list of quadruplets.
# Evaluates expressions using '+ - * / % ( )'. All other characters are 
# removed. A vertex count is added to the beginning.
# Also does the nav data file. Accepts string too.
#


BEGIN	{
	hextab["0"] = 0
	hextab["1"] = 1
	hextab["2"] = 2
	hextab["3"] = 3
	hextab["4"] = 4
	hextab["5"] = 5
	hextab["6"] = 6
	hextab["7"] = 7
	hextab["8"] = 8
	hextab["9"] = 9
	hextab["a"] = 10
	hextab["b"] = 11
	hextab["c"] = 12
	hextab["d"] = 13
	hextab["e"] = 14
	hextab["f"] = 15
	hextab["A"] = 10
	hextab["B"] = 11
	hextab["C"] = 12
	hextab["D"] = 13
	hextab["E"] = 14
	hextab["F"] = 15

	fin  = ARGV[1]
	fout = ARGV[2]

	fname = fin

	lineno = 0
	advance()
	while (1) {
		if (tok ~ /^".*/) {
			printf ("%s\n", tok) >fout
			advance()
		} else {
			x = int(expression())
			printf ("%d\n", x) >fout
		}
		++n
		if (tok == "(eof)")
			break;
		eat(",")
	}
	
	close(fin)
	close(fout)
}

function advance() {
	if (tok == "(eof)")
		return tok
	while (1) {
		while (1) {
			if (length(line) == 0)
				++lineno
			else if (substr(line,1,1) == "#") {
				split(line,a)
				if (a[1] == "#line") {
					lineno = a[2]+0
				} else if (a[1] == "#") {
					lineno = a[2]+0
					if (a[3] != "")
						fname = a[3]
				} else
					++lineno
			} else
				break
			if (getline line <fin == 0)
				return tok = "(eof)"
		}
		if (match(line, /^[ \t;\n{}]/))
		    	line = substr(line, 2)
		else
		    	break
	}
	if (match(line, /^0[xX][0-9a-fA-F]+/) || match(line, /^[.0-9]+/) ||
	    match(line, /^(<=|==|!=|>=|\|\||&&|>>|<<)/) ||
	    match(line, /^".*"/) ||
	    match(line, /^./)) {
	    	tok = substr(line, 1, RLENGTH)
	    	line = substr(line, RLENGTH+1)
	    	return tok
	}
	error(fname "(" lineno ") unrecognized token")
}

function eat(s) {
	if (tok != s)
		error(fname "(" lineno ") saw \"" tok "\" expected \"" s "\"")
	advance()
}

function error(s) {
	print "Error: " s
	exit 1
}

function statement(  e, e1, e2) {
	return
}

function expression(  e, e1, e2) {
	e = logical_or()
	if (tok == "?") {
		advance()
		e1 = expression()
		eat(":")
		e2 = expression()
		if (e)
			e = e1
		else
			e = e2
# This fails!:	e = e ? e1 : e2
	}
	return e
}

function logical_or(  e, e1) {
	e = logical_and()
	while (tok == "||") {
		advance()
		e1 = logical_and()
		e = e || e1
	}
	return e
}

function logical_and(  e, e1) {
	e = inclusive_or()
	while (tok == "&&") {
		advance()
		e1 = inclusive_or()
		e = e && e1
	}
	return e
}

function bitwise(a, b, op,  abit, bbit, asign, bsign, rbit, r, t, n) {
	a = int(a)
	asign = (a < 0) ? 1 : 0;
	b = int(b)
	bsign = (b < 0) ? 1 : 0;
	r = 0
	rpos = 1
	for (n = 0; n < 32; ++n) {
		t = int((a-asign)/2)
		abit = a != t+t
		a = t
		t = int((b-bsign)/2)
		bbit = b != t+t
		b = t
		if (op == "|")
			rbit = abit || bbit
		else if (op == "&")
			rbit = abit && bbit
		else if (op == "^")
			rbit = abit != bbit
		else # unary ~
			rbit = !bbit
		r += rpos*rbit
		rpos += rpos
	}
	return r
}

function inclusive_or(  e, e1) {
	e = exclusive_or()
	while (tok == "|") {
		advance()
		e1 = exclusive_or()
		e = bitwise(e, e1, "|");
	}
	return e
}

function exclusive_or(  e, e1) {
	e = and()
	while (tok == "^") {
		advance()
		e1 = and()
		e = bitwise(e, e1, "^");
	}
	return e
}

function and(  e, e1) {
	e = equality()
	while (tok == "&") {
		advance()
		e1 = equality()
		e = bitwise(e, e1, "&");
	}
	return e
}

function equality(  op, e, e1) {
	e = relational()
	while (tok == "==" || tok == "!=") {
		op = tok
		advance()
		e1 = relational()
		if (op == "==")
			e = e == e1
		else
			e = e != e1
	}
	return e
}

function relational(  op, e, e1) {
	e = shift()
	while (tok ~ /<|<=|>=|>/) {
		op = tok
		advance()
		e1 = shift()
		if (op == "<")
			e = e < e1
		else if (op == "<=")
			e = e <= e1
		else if (op == ">=")
			e = e >= e1
		else
			e = e > e1
	}
	return e
}

function shift(  op, e, e1) {
	e = additive()
	while (tok == "<<" || tok == ">>") {
		op = tok
		advance()
		e1 = additive()
		if (op == ">>")
			e1 = -e1
		if (e1 > 0) {
			for (e1 = int(e1); e1 > 0; --e1)
				e *= 2;
		} else {
			for (e1 = int(-e1); e1 > 0; --e1)
				e /= 2;
		}
	}
	return e
}

function additive(  op, e, e1) {
	e = multiplicative()
	while (tok == "+" || tok == "-") {
		op = tok
		advance()
		e1 = multiplicative()
		if (op == "+")
			e = e + e1
		else
			e = e - e1
	}
	return e
}

function multiplicative(  op, e, e1) {
	e = unary()
	while (tok == "*" || tok == "/" || tok == "%") {
		op = tok
		advance()
		e1 = unary()
		if (op == "*")
			e = e * e1
		else if (op == "/")
			e = e / e1
		else
			e = e % e1
	}
	return e
}

function unary() {
	if (tok == "-") {
		advance()
		return -primary()
	} else if (tok == "~") {
		advance()
		return bitwise(0, primary(), "~")
	} else if (tok == "!") {
		advance()
		return !primary()
	} else if (tok == "+") {
		advance()
		return primary()
	} else
		return primary()
}

function primary(  e) {
	if (tok == "(") {
		advance()
		e = expression()
		eat(")")
		return e
	}

	if (tok ~ /^0[xX][0-9a-fA-F]+/) {
		e = gethex(tok)
		advance()
		return e+0
	}

	if (tok ~ /^[.0-9]+/) {
		e = tok
		advance()
		return e+0
	}

	error(fname "(" lineno ") unexpected \"" tok "\"")
}

function gethex(tok) {
	n = length(tok)
	e = 0
	for (i = 3; i <= n; ++i) {
		c = substr(tok,i,1)
		if (c in hextab)
			c = hextab[c]
		else
			error(fname "(" lineno ") bad hex number \"" tok "\"")
		e = e*16 + c
	}
	return (e)
}
