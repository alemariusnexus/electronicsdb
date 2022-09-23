-- Adapted from: https://gist.github.com/liukun/f9ce7d6d14fa45fe9b924a3eed5c3d99

local urlenc = {}

local char_to_hex = function(c)
    return string.format("%%%02X", string.byte(c))
end

function urlenc.urlencode(url)
    if url == nil then
        return
    end
    url = url:gsub("\n", "\r\n")
    url = url:gsub("([^%w ])", char_to_hex)
    url = url:gsub(" ", "+")
    return url
end

local hex_to_char = function(x)
    return string.char(tonumber(x, 16))
end

urlenc.urldecode = function(url)
    if url == nil then
        return
    end
    url = url:gsub("+", " ")
    url = url:gsub("%%(%x%x)", hex_to_char)
    return url
end

return urlenc
