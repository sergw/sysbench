local console = require('console')
console.listen("/usr/local/var/run/tarantool/sysbench-server.control")

box.cfg {
    pid_file   = "/usr/local/var/run/tarantool/sysbench-server.pid",
    wal_dir    = "/usr/local/var/lib/tarantool/sysbench-server",
    memtx_dir  = "/usr/local/var/lib/tarantool/sysbench-server",
    log        = "/usr/local/var/log/tarantool/sysbench-server.log",
    username   = "root",
    listen = 3301,
    memtx_memory = 2000000000,
    checkpoint_interval = 0,
    background = true,
    wal_mode = 'none',
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



