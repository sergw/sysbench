local console = require('console')
console.listen("/tmp/sysbench-server.sock")

box.cfg {
    pid_file   = "./sysbench-server.pid",
    log        = "./sysbench-server.log",
    listen = 3301,
    memtx_memory = 2000000000,
    background = true,
    checkpoint_interval = 0,
    wal_mode = 'none',
}


function try(f, catch_f)
    local status, exception = pcall(f)
    if not status then
        catch_f(exception)
    end
end

try(function()
    box.schema.user.grant('guest', 'read,write,execute', 'universe')
end, function(e)
    print(e)
end)



