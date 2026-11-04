
t={"a"=1,"b"=2,"c"=3}
t2=table.copy(t)

if t==t2 then
	print("table.copy failed test, copied table is the same object.")
	return 1
else
	passed=true
	iterate(t,function(key,val,obj)
		if t2[key]!=val then passed=false return 1 end
	end)
	if !!passed then
		print("table.copy failed test, copied table has differing data.")
		return 1
	end
end


emptytable={}
filledtable={"a"=1,"b"=2,"c"=nil}

if table.haskey(emptytable,"c") then
	print("table.haskey failed test , empty table has a key registered in it.")
	return 1
end

if !!table.haskey(filledtable,"c") then
	print("table.haskey failed test , table with empty key association did not register the key.")
	return 1
end

if !!table.haskey(filledtable,"b") then
	print("table.haskey failed test , table with filled key association did not register the key.")
	return 1
end

if table.hasvalue(emptytable, 2) then
	print("table.hasvalue failed test , empty table has a value registered in it.")
	return 1
end

if !!table.hasvalue(filledtable,nil) then
	print("table.hasvalue failed test , table with nil valuedid not register the value.")
	return 1
end

if !!table.hasvalue(filledtable, 2) then
	print("table.hasvalue failed test , table with value did not register the value.")
	return 1
end


local keys,kcount=table.getkeys(filledtable)
local values,vcount=table.getvalues(filledtable)

if vcount !=kcount then
	print("table.getkeys did not return the same count as table.getvalues")
	return 1
end

n=0
while n<kcount do
	if filledtable[keys[n]]!=values[n] then
		print(string.format("table.getkeys and table.getvalues did not agree on elements stored. key: %s stored value: %s returned value: %s, element number %i",keys[n],filledtable[keys[n]],values[n],n) )
		return 1
	end
	n++
end


keys,num=table.keyswithvalue(filledtable,2)
if num!=1 then
	print("table.keyswithvalue returned incorrect number variable, got "..num.." expected 1")
	return 1
end
if keys[0]!="b" then
	print("table.keyswithvalue returned incorrect array element, got ".. keys[0] .." expected b")
	return 1
end

table.flush(filledtable)

if filledtable.c then
	print("table.flush set nil key association to incorrect value "..filledtable.c)
	return 1
end

if #filledtable != 2 then
	print("table.flush resulted in wrong length. expected 2, got ".. #filledtable)
	return 1
end

if table.haskey(filledtable,"c") then
	print("table.flush did not properly clear the key.")
	return 1
end
if table.hasvalue(filledtable,nil) then
	print("table.flush did not properly clear the value.")
	return 1
end
