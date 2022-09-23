local urlencode = require("urlencode")

local prov = {}

function prov.queryPartProductURL(part, urls)
    local notes = PartGetPropertyValue(part, "notes")

    for partNo in notes:lower():gmatch("reichelt partno: ([^\n]*)") do
        partNo = partNo:gsub("^%s*(.-)%s*$", "%1")
        table.insert(urls, "https://www.reichelt.com/index.html?ACTION=446&q=" .. urlencode.urlencode(partNo))
    end
end

return prov
