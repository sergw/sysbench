box.cfg {
    listen = 3301,
    memtx_memory = 5000000000,
}

function try(f, catch_f)
    local status, exception = pcall(f)
    if not status then
        catch_f(exception)
    end
end

try(function()
    box.schema.user.grant('guest', 'read,write', 'universe')
end, function(e)
    print(e)
end)



