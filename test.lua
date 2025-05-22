-- test gdbm library

local gdbm=require"gdbm"

-- support code --------------------------------------------------------------

gdbm.version=string.gsub(gdbm.version,"using .* version (.-)[%.,] .*$","using GDBM %1")

do

local M= {
	__index=function (t,k) return t.gdbm:fetch(k) end,
	__newindex=function (t,k,v)
		if v~=nil then t.gdbm:replace(k,v) else t.gdbm:delete(k) end
	end
}

function gdbm.proxy(t)
	return setmetatable({gdbm=t},M)
end

local function keys(d,k)
	if k==nil then return d:firstkey() else return d:nextkey(k) end
end

function gdbm.keys(d)
	return keys,d
end

local function entries(d,k)
	local v
	if k==nil then k=d:firstkey() else k=d:nextkey(k) end
	if k==nil then v=nil else v=d:fetch(k) end
	return k,v
end

function gdbm.entries(d)
	return entries,d
end

end

------------------------------------------------------------------------------

function testing(s)
	print""
	print("-------------------------------------------------------",s)
end

function gdbm.show(d)
	local n=0
	for k,v in d:entries() do
		n=n+1
		print(n,k,v)
	end
	return d
end

------------------------------------------------------------------------------
print(gdbm.version)

F="test.gdbm"
O=F
local d=assert(gdbm.open(F,"n"))

------------------------------------------------------------------------------
testing"insert method"
d:insert("JAN","January")
d:insert("FEB","February")
d:insert("MAR","March")
d:insert("APR","April")
d:insert("MAY","May")
d:insert("JUN","June")
d:show()

------------------------------------------------------------------------------
testing"proxy insert"
t=d:proxy()
t.JUL="July"
t.AUG="August"
t.SEP="September"
t.OCT="October"
t.NOV="November"
t.DEC="December"
d:show()

------------------------------------------------------------------------------
testing"proxy delete"
t.JAN=nil
t.FEB=nil
t.MAR=nil
t.APR=nil
t.MAY=nil
t.JUN=nil
d:show()

------------------------------------------------------------------------------
testing"reorganize"
if d.reorganize then
	for i=1,1000 do t[i]=tostring(i) end
	os.execute("ls -l "..F)
	for i=1,1000 do t[i]=nil end
	print("reorganize",d:reorganize())
	os.execute("ls -l "..F)
else
	print("not available")
end

------------------------------------------------------------------------------
testing"return codes"
print("JAN exists?",d:exists("JAN"))
print("replace JAN?",d:replace("JAN","Janeiro"))
print("JAN is now",t.JAN)
print("DEC exists?",d:exists("DEC"))
print("replace DEC?",d:replace("DEC","Dezembro"))
print("DEC is now",t.DEC)
print("insert JUL?",d:insert("JUL","Julho"))
print("insert XXX?",d:insert("XXX","Julho"))
print("JUL is",d:fetch("JUL"))
print("XXX is",d:fetch("XXX"))
print("YYY is",d:fetch("YYY"))
print("insert XXX",d:insert("XXX","a"))
print("insert XXX",d:insert("XXX","b"))
print("XXX is",d:fetch("XXX"))
print("replace XXX",d:replace("XXX","1"))
print("replace XXX",d:replace("XXX","2"))
print("XXX is",d:fetch("XXX"))
print("delete XXX",d:delete("XXX"))
print("delete XXX",d:delete("XXX"))

if d.lasterror then
	print("lasterror",d:lasterror())
end

------------------------------------------------------------------------------
testing"sync"
print(d:sync())

------------------------------------------------------------------------------
testing"method chaining"
d:insert("FEB","Fevereiro"):delete("JUL"):replace("NOV","Novembro"):show()

------------------------------------------------------------------------------
testing"counting"
if d.count then
	print(d:count(),"entries")
else
	print("not available")
end

------------------------------------------------------------------------------
testing"export/import"
if d.export and d.import then
	F="test.flat"
	print("export  ",d:export(F))
	d:delete("NOV")
	d:delete("DEC")
	d:show()
	print("import store",d:import(F))
	print("import replace",d:import(F,true))
  os.execute("rm -rf "..F)
else
	print("not available")
end
d:show()

------------------------------------------------------------------------------
testing"dump/load"
if d.dump and d.load then
	d:insert("new","value")
	F="test.txt"
	d:show()
	print("dump  ",d:dump(F))
	--d:delete("NOV")
	--d:delete("DEC")
	--d:show()
	print("load store",d:load(F))
	print("load replace",d:load(F,true))
  os.execute("rm -rf "..F)
else
	print("not available")
end
d:show()

------------------------------------------------------------------------------
testing"needsrecovery"
if d.needsrecovery then
	print("needsrecovery",d:needsrecovery())
	print("recover ",d:recover())
	print("lasterror",d:lasterror())
else
	print("not available")
end

------------------------------------------------------------------------------
testing"close"
print("opened",d)
d:close()
print("closed",d)

------------------------------------------------------------------------------
print""
print(gdbm.version)

os.execute("rm -rf "..O)
