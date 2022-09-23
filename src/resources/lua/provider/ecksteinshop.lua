local urlencode = require("urlencode")

local prov = {}

function prov.queryPartProductURL(part, urls)
    local notes = PartGetPropertyValue(part, "notes")

    for partNo in notes:lower():gmatch("eckstein partno: ([^\n]*)") do
        partNo = partNo:gsub("^%s*(.-)%s*$", "%1")
        table.insert(urls, "https://eckstein-shop.de/index.php?qs=" .. urlencode.urlencode(partNo))
    end
end

return prov
