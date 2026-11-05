local aprox=function(value,target) return math.abs(value-target)<0.000001 end

if (math.int_max+1)&math.int_max!=0 then
	print("math.int_max is an incorrect value!")
	return 1
end
if math.uint_max+1 !=0 then
	print("math.uint_max is an incorrect value!")
	return 1
end

if math.min()!=inf then
	print("math.min has in incorrect starting value!")
	return 1
end

if math.max()!=-inf then
	print("math.max has in incorrect starting value!")
	return 1
end

if math.min(13,1,8,4,3.3,12.9,1.7,0.5)!=0.5 then
	print("math.min returned incorrect value!")
	return 1
end

if math.max(13,1,8,4,3.3,12.9,1.7,0.5)!=13 then
	print("math.max returned incorrect value!")
	return 1
end

if math.floor(11.8)!=11 then
	print("math.floor returned incorrect value!")
	return 1
end

if math.ceil(10.1)!=11 then
	print("math.ceil returned incorrect value!")
	return 1
end

if (math.abs(-12)!=12) || (math.abs(-3.7)!=3.7) then
	print("math.abs returned incorrect value!")
	return 1
end

if math.radians(180.0)!=math.pi then
	print("math.radians returned incorrect value!")
	return 1
end
if math.degrees(math.pi)!=180.0 then
	print("math.degrees returned incorrect value!")
	return 1
end

if math.log(8.0,2.0)!=3.0 then
	print("math.log returned incorrect value!")
	return 1
elseif math.log(math.e^2.0)!=2.0 then
	print("math.log using wrong default base!")
	return 1
end

if (!!aprox(math.sin(0.0),0.0)) || (!!aprox(math.sin(0.5*math.pi),1.0)) then
	print("math.sin returned incorrect value!")
	return 1
end
if (!!aprox(math.cos(0.0),1.0)) || (!!aprox(math.cos(0.5*math.pi),0.0)) then
	print("math.cos returned incorrect value!")
	return 1
end
if (!!aprox(math.tan(0.0),0.0)) || (!!aprox(math.tan(0.25*math.pi),1.0)) then
	print("math.tan returned incorrect value!")
	return 1
end

if (!!aprox(math.asin(0.0),0.0)) || (!!aprox(math.asin(1.0),0.5*math.pi)) then
	print("math.asin returned incorrect value!")
	return 1
end
if (!!aprox(math.acos(1.0),0.0)) || (!!aprox(math.acos(0.0),0.5*math.pi)) then
	print("math.acos returned incorrect value!")
	return 1
end
if (!!aprox(math.atan(0.0),0.0)) || (!!aprox(math.atan(1.0),0.25*math.pi)) then
	print("math.atan returned incorrect value!")
	return 1
end


if !!aprox(math.atan2(0.0,0.0),0.0) then
	print("math.atan2 returned incorrect value at extreme 0,0!")
	return 1
elseif !!aprox(math.atan2(0.0,1.0), 0.0 ) then
	print("math.atan2 returned incorrect value at extreme 1,0!")
	return 1
elseif !!aprox(math.atan2(0.0,-1.0), math.pi ) then
	print("math.atan2 returned incorrect value at extreme -1,0!")
	return 1
elseif !!aprox(math.atan2(1.0,0.0), math.pi/2 ) then
	print("math.atan2 returned incorrect value at extreme 0,1!")
	return 1
elseif !!aprox(math.atan2(-1.0,0.0), math.pi/-2 ) then
	print("math.atan2 returned incorrect value at extreme 0,-1!")
	return 1
elseif !!aprox(math.atan2(1.0,1.0), math.pi/4 ) then
	print("math.atan2 returned incorrect value at value 1,1!")
	return 1
end

if !!math.isnan(0/0) then
	print("math.isnan returned incorrect value for NaN.")
	return 1
elseif !!math.isnan(1/0) then
	print("math.isnan returned incorrect value for Infinity.")
	return 1
elseif math.isnan(100) then
	print("math.isnan returned incorrect value for integer value.")
	return 1
elseif math.isnan(1.00154) then
	print("math.isnan returned incorrect value for float value.")
	return 1
elseif math.isnan("test") then
	print("math.isnan returned incorrect value for string value.")
	return 1
end

local initial=1004320
math.randomseed(initial)
if math.randomseed()!=initial then
	print("math.randomseed did not preserve seed state.")
	return 1
end

if !!aprox(math.stddev(1,2,2,3,3,4,4,5,5,5,1),1.53741222957) then
	print("math.stddev returned incorrect result")
end
if !!aprox(math.astddev([1,2,2,3,3,4,4,5,5,5,1]),1.53741222957) then
	print("math.astddev returned incorrect result")
end

if !!aprox(math.median(1,2,2,3,3,4,4,5,5,5,1),3.0) then
	print("math.median returned incorrect result")
end
if !!aprox(math.amedian([1,2,2,3,3,4,4,5,5,5,1]),3.0) then
	print("math.amedian returned incorrect result")
end
