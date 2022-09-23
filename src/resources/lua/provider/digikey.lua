local urlencode = require("urlencode")

local prov = {}

function prov.queryPartProductURL(part, urls)
    local notes = PartGetPropertyValue(part, "notes")

    for partNo in notes:lower():gmatch("digi[-]?key partno: ([^\n]*)") do
        partNo = partNo:gsub("^%s*(.-)%s*$", "%1")
        table.insert(urls, "https://www.digikey.com/en/products/result?keywords=" .. urlencode.urlencode(partNo))
    end
end

return prov
