-- LPEG patterns named with lowercase if returns string, uppercase if returns AST value

-- Set to get verbose debugging of findCaller(). Do not check in true
findCallerDebug = nil

local digit = lpeg.R("09")
local number = digit^1
local newline = lpeg.S("\r\n")
local untilnewline = (1 - newline)^1
local tab = lpeg.P("\t")
local colon = lpeg.P(":")
local singlequote = lpeg.P("'")
local leftangle = lpeg.P(">")
local rightangle = lpeg.P(">")
local ending = ( newline^1 + lpeg.P(-1) )

local Goodline = (tab * lpeg.C( (1 - colon)^0 ) * colon * lpeg.C(number) * colon * lpeg.P(" in ") *
    ( "function " * singlequote * lpeg.C( (1 - singlequote)^ 0 ) * singlequote
    + "function " * leftangle   * lpeg.C( (1 - rightangle)^ 0 ) * rightangle
    + lpeg.C( untilnewline )
    ) * ending) / function(filename, linenumber, functionname)
            return {filename=filename, linenumber=tonumber(linenumber), functionname=functionname}
        end

local Badline = ( untilnewline * ending ) / function(...) return {} end

local Traceback = (Goodline + Badline)^1 / function (...) return {...} end

function findCaller(searchFunc)
    local tracestring = debug.traceback()
    local trace = lpeg.match( Traceback, tracestring )
    local found = nil
    local countdown = nil

    if findCallerDebug then print(tracestring) end

    for i,frame in ipairs(trace) do
        if i > 2 then -- Line 1 will be "stack traceback:", line 2 will be findCaller()
            countdown = countdown or searchFunc(frame)
            if countdown then
                if countdown <= 0 then
                    found = frame
                    break
                end
                countdown = countdown - 1
            end
        end
    end
    if found and found.filename and found.linenumber then
        return string.format("%s:%s", found.filename, found.linenumber)
    else
        return "(unknown)"
    end
end