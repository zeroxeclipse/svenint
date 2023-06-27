function OnFirstClientdataReceived(receive_time)
    printl( "OnFirstClientdataReceived(" .. receive_time .. ")" );
    
    if ( SpeedrunStart ~= nil ) then SpeedrunStart() end
    
    ClientCmd( "record autorecord_" .. MapName );
    -- ClientCmd( "wait; use weapon_gauss" );
    -- ClientCmd( "wait; use weapon_medkit" );
end

function OnBeginLoading()
    printl( "OnBeginLoading()" );
end

function OnEndLoading()
    printl( "OnEndLoading()" );
    ClientCmd( "stop; record autorecord_" .. MapName );
end

function OnDisconnect()
    printl( "OnDisconnect()" );
end

-- function OnClientPutInServer( pPlayerEdict )
    -- printl( "OnClientPutInServer( " .. pPlayerEdict.vars.netname .. " )" );

    -- SendCommandToClient( pPlayerEdict, "sc_st_legit_mode" );
-- end

function IncludeMapScript( mapname )
    return false;
end

if ( IncludeMapScript( MapName ) ) then
    printl( "Executed map script \"" .. MapName .. "\"" );
end

printl( "Executed \"main.lua\"" );