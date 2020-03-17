local xml_string_tb = {
["quot;"] = "\"",
["apos;"] = "'",
["gt;"] = ">",
["lt;"] = "<",
["amp;"] = "&",
}

function FromXmlString(value)
    value = string.gsub(value, "&#x([%x]+)%;",
        function(h)
            return string.char(tonumber(h, 16))
        end);
    value = string.gsub(value, "&#([0-9]+)%;",
        function(h)
            return string.char(tonumber(h, 10))
        end);
    value = string.gsub(value, "%&(%w+;)", xml_string_tb);
    --value = string.gsub(value, "&apos;", "'");
    --value = string.gsub(value, "&gt;", ">");
    --value = string.gsub(value, "&lt;", "<");
    --value = string.gsub(value, "&amp;", "&");
    return value;
end

function parseargs(s)
  local arg = {}
  string.gsub(s, "([%-%w]+)=([\"'])(.-)%2", function (w, _, a)
    arg[w] = FromXmlString(a)
  end)
  return arg
end

function parse_tag(s, pos)
    local ni,j,c,label = string.find(s, "<([%/!]?)([%w:%-_]+)", pos)
    if not j then return end
    if c == "!" and label:sub(1,2) == "--" then
        return ni,pos+1,c,label,"",""
    end
    local p1,p2,xarg = string.find(s, "%s+([%-%w_]+)", j+1)
    local empty = ""
    if p1 and string.find(s, ">", j+1) < p1 then 
        p1 = nil
    end
    if p1 then
       local v = ""
       while p1 do
            p1,p2,v = string.find(s, "(%s*=%s*\"[^\"]*\")", p2+1)
            if p1 then xarg = xarg .. v .. " "
            else break end
            j = p2
            p1,p2,v = string.find(s, "%s+([%-%w_]+)", p2+1)
            
            if p1 then
                if string.find(s, ">", j+1) < p1 then break end
                xarg = xarg .. v
            else break end
            
            j = p2
       end
    else
        xarg = ""
    end
    p1,p2,empty = string.find(s, "(%/?)>", j+1)
    j = p2
    return ni,j,c,label,xarg, empty
end

function collect(s)
  local stack = {}
  local top = {}
  table.insert(stack, top)
  local ni,c,label,xarg, empty
  local i, j = 1, 1
  while true do
    --ni,j,c,label,xarg, empty = string.find(s, "<([%/!]?)([%w:-]+)(.-)(%/?)>", i)
    ni,j,c,label,xarg, empty = parse_tag(s,i)
    --log(ni,j,c,label,xarg, empty)
    if not ni then break end
    local text = string.sub(s, i, ni-1)
    if not string.find(text, "^%s*$") then
      table.insert(top, text)
    end
    if empty == "/" then  -- empty element tag
      table.insert(top, {label=label, xarg=parseargs(xarg), empty=1})
    elseif c == "!" and label:sub(1,2) == "--" then -- comment tag
      while true do
        ni, j = string.find(s, "%-%->", i)
        --log("comment end:",ni,j, '"'..s:sub(ni,j)..'"')
        if j-ni == 2 then break end
        i = j + 1
      end
    elseif c == "" then   -- start tag
      top = {label=label, xarg=parseargs(xarg)}
      table.insert(stack, top)   -- new level
    else  -- end tag
      local toclose = table.remove(stack)  -- remove top
      top = stack[#stack]
      if #stack < 1 then
        error("nothing to close with "..label)
      end
      if toclose.label ~= label then
        log(ni,j,c,label,xarg, empty)
        error("trying to close "..toclose.label.." with "..label.." at "..i
              .."\r\n"..string.sub(s, i-150, j+150))
      end
      table.insert(top, toclose)
    end
    i = j+1
  end
  local text = string.sub(s, i)
  if not string.find(text, "^%s*$") then
    table.insert(stack[#stack], text)
  end
  if #stack > 1 then
    error("unclosed "..stack[#stack].label)
  end
  return stack[1]
end
