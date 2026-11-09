global ASCII_ENABLED=true

global decoratetext=function(text,color_fg,color_bg)
	if !!ASCII_ENABLED then return text end
	//if color_fg then text=string.format("\e[%im%s",color_fg+29,text) end
	if color_fg then text="\e["..(color_fg+29).."m"..text end
	if color_bg then text="\e["..(color_bg+39).."m"..text end
	//if color_bg then text=string.format("\e[%im%s",color_bg+39,text) end
	//if underline then text="\e[4m"..text end
	return text.."\e[0m"
end

global clear_screen=function()
	if global os.IS_UNIX then
		global os.call("clear")
	else
		global os.call("cls")
	end
end

global xy_to_array=function(x,y)
	return (x+(y*global size_x))
end

global xy_is_valid=function(x,y)
	if (x<0) || (y<0) then return false end
	if (x>=size_x) || (y>=size_y) then return false end
	return true
end

global get_tile_numbercount=function(x,y)
	local idx=(x+(y*global size_x))
	if gameboard[idx] then return -1 end
	local found=0
	
	if xy_is_valid(x+1,y) && gameboard[xy_to_array(x+1,y)] then found++ end
	if xy_is_valid(x-1,y) && gameboard[xy_to_array(x-1,y)] then found++ end
	if xy_is_valid(x,y+1) && gameboard[xy_to_array(x,y+1)] then found++ end
	if xy_is_valid(x,y-1) && gameboard[xy_to_array(x,y-1)] then found++ end
	if xy_is_valid(x+1,y+1) && gameboard[xy_to_array(x+1,y+1)] then found++ end
	if xy_is_valid(x-1,y+1) && gameboard[xy_to_array(x-1,y+1)] then found++ end
	if xy_is_valid(x+1,y-1) && gameboard[xy_to_array(x+1,y-1)] then found++ end
	if xy_is_valid(x-1,y-1) && gameboard[xy_to_array(x-1,y-1)] then found++ end
	
	return found
end

global reveal_tile=function(x,y,flag)
	if !!xy_is_valid(x,y) then return end
	if flag==nil then flag=50 end
	local idx=(x+(y*global size_x))
	if visible[idx]!=VISIBLE_UNKNOWN then return end
	if gameboard[idx] then 
		//you lost the game
		GAMESTATE=STATE_LOSE
	end
	local n=get_tile_numbercount(x,y)
	if n==0 then
		visible[idx]=VISIBLE_VISIBLE
		if flag then
			flag--
			reveal_tile(x+1,y,flag)
			reveal_tile(x+1,y+1,flag)
			reveal_tile(x,y+1,flag)
			reveal_tile(x-1,y,flag)
			reveal_tile(x-1,y-1,flag)
			reveal_tile(x,y-1,flag)
			reveal_tile(x-1,y+1,flag)
			reveal_tile(x+1,y-1,flag)
		end
	elseif n>0 then
		visible[idx]=n
	else
		visible[idx]=VISIBLE_BOMBREVEALED
	end
	return true
end

global COLOR_BLACK=1
global COLOR_RED=2
global COLOR_GREEN=3
global COLOR_YELLOW=4
global COLOR_BLUE=5
global COLOR_MAGENTA=6
global COLOR_CYAN=7
global COLOR_WHITE=8



io.post(decoratetext("RED ON CYAN",COLOR_RED,COLOR_CYAN).." "..decoratetext("YELLOW ON MAGENTA",COLOR_YELLOW,COLOR_MAGENTA).."\n"    ) 
print()
print("if your terminal supports ANSI escaping, the text above should appear with some formatting. if it doesn't, then the game will not work correctly.\n\nhit enter to proceed")
if string.lower(io.prompt()[1])!="y" then
	ASCII_ENABLED=false
end
ASCII_ENABLED=true

start:
clear_screen()
print("[1] easy (7x7 10 bombs)")
print("[2] medium (11x11 20 bombs)")
print("[3] hard (15x15 40 bombs)")
print("[4] custom")
retry:
global bombs=0
global size_x=0
global size_y=0
n=io.prompt()
if n=="1" then
	bombs=10
	size_x=7
	size_y=7
elseif n=="2" then
	bombs=20
	size_x=11
	size_y=11
elseif n=="3" then
	bombs=40
	size_x=15
	size_y=15
elseif n=="4" then
	while !!size_x do
		io.post("X board size:")
		size_x=toint(io.prompt())
	end
	
	while !!size_y do
		io.post("y board size:")
		size_y=toint(io.prompt())
	end
	
	while !!bombs do
		io.post("number of bombs:")
		bombs=toint(io.prompt())
	end
elseif n=="\x03" then
	os.exit(1)
else
	goto retry
end




cursorx=0
cursory=0
global FIELD_EMPTY=0
global FIELD_BOMB=1

global VISIBLE_UNKNOWN=0
global VISIBLE_FLAG=-1
global VISIBLE_BOMBREVEALED=-2
global VISIBLE_VISIBLE=-3


global STATE_PLAY=0
global STATE_WIN=1
global STATE_LOSE=-1

global color_lookups=[
1=COLOR_BLUE,
2=COLOR_YELLOW,
3=COLOR_YELLOW,
4=COLOR_MAGENTA,
5=COLOR_RED,
6=COLOR_RED,
7=COLOR_WHITE,
8=COLOR_WHITE,
]

randomoffset=0 //so consecutive calls don't go over the same shit
reseed:
global gameseed=math.randomseed(os.time()+randomoffset)
randomoffset++
local firstdig=true

global GAMESTATE=STATE_PLAY

gameboard=[]
visible=[]
tiles=[]
n=size_x*size_y
while n do
	n--
	gameboard[n]=FIELD_EMPTY
	visible[n]=VISIBLE_UNKNOWN
	tiles[#tiles]=n
end




b=bombs
while b do
	place=array.remove(tiles,math.randomint(0,#tiles-1) )
	gameboard[place]=FIELD_BOMB
	b--
end



while true do
	local flagcount=0
	local revealedtiles=0
	
	//print(inp)
	//draw field
	local OUTPUT_BUFFER=""
	local OUTPUT_BUF_ARR=[]
	local y=size_y
	while y do
		y--
		local x=size_x
		while x do
			x--
			local status=visible[(x+(y*global size_x))]
			local hoverover= (cursorx==x) && (cursory==y)
			if status == VISIBLE_UNKNOWN then
				OUTPUT_BUF_ARR[#OUTPUT_BUF_ARR]=( decoratetext("?",COLOR_WHITE,hoverover ? COLOR_GREEN : COLOR_WHITE) )
			elseif status == VISIBLE_BOMBREVEALED then
				OUTPUT_BUF_ARR[#OUTPUT_BUF_ARR]=( decoratetext("*",COLOR_RED,hoverover ? COLOR_GREEN : COLOR_RED) )
			elseif status == VISIBLE_FLAG then
				OUTPUT_BUF_ARR[#OUTPUT_BUF_ARR]=( decoratetext("!",COLOR_RED,hoverover ? COLOR_GREEN : COLOR_WHITE) )
				flagcount++
			elseif status == VISIBLE_VISIBLE then
				OUTPUT_BUF_ARR[#OUTPUT_BUF_ARR]=( decoratetext(" ",COLOR_BLACK,hoverover ? COLOR_GREEN : COLOR_BLACK) )
				revealedtiles++
			else
				
				OUTPUT_BUF_ARR[#OUTPUT_BUF_ARR]=( decoratetext((status),color_lookups[status],hoverover ? COLOR_GREEN : COLOR_BLACK))
				revealedtiles++
			end
			
			
			
			
			
		end
		OUTPUT_BUF_ARR[#OUTPUT_BUF_ARR]=("\n")
	end
	
	if revealedtiles==(size_x*size_y-bombs) then
		GAMESTATE=STATE_WIN
	end
	
	OUTPUT_BUFFER=array.concat(OUTPUT_BUF_ARR,"")
	
	clear_screen()
	
	io.post(OUTPUT_BUFFER)
	
	if GAMESTATE == STATE_PLAY then 
		io.post(string.format("enter = flag\tspace = dig %i/%i\n",flagcount,bombs ))
	elseif GAMESTATE==STATE_WIN then
		io.post("you won!\nhit enter to start new game.")
		io.prompt()
		clear_screen()
		goto start
	else
		io.post("you lost!\nhit enter to start new game.")
		io.prompt()
		clear_screen()
		goto start
	end
	
	
	
	inp=nil
	if !!trydignext then inp=io.input() end
	if inp=="\x03" then os.exit() end
	//print(string.toarray(inp)[0])
	if (inp=="\xE0H") || (inp=="\e\x5b\x41") then cursory=cursory+1 end //up arrow
	if (inp=="\xE0K") || (inp=="\e\x5b\x44") then cursorx=cursorx+1 end //left arrow
	if (inp=="\xE0M") || (inp=="\e\x5b\x43") then cursorx=cursorx-1 end //right arrow
	if (inp=="\xE0P") || (inp=="\e\x5b\x42") then cursory=cursory-1 end //down arrow
	cursory=math.max(math.min(size_y-1,cursory),0)
	cursorx=math.max(math.min(size_x-1,cursorx),0)
	if (inp=="\r") || (inp=="\n") then //enter
		idx=xy_to_array(cursorx,cursory)
		if visible[idx]==VISIBLE_UNKNOWN then
			visible[idx]=VISIBLE_FLAG
		elseif visible[idx]==VISIBLE_FLAG then
			visible[idx]=VISIBLE_UNKNOWN
		end
	end
	if inp==" " || trydignext then //space
		trydignext=false
		if reveal_tile(cursorx,cursory) then
			idx=xy_to_array(cursorx,cursory)
			if visible[idx]!=VISIBLE_VISIBLE && firstdig then trydignext=true goto reseed end
			firstdig=false
		end
	end
end

