function OnTouchTrigger(name)
	printl( "Touched trigger: \"" .. name .. "\"" );
end

if ( GetMapName():lower() == "hl_c01_a1" ) then
	CreateTrigger("test_trigger", Vector(104.531, 169.531, -209.969), Vector(0, 0, 0), Vector(98.281, 210.438, 78.991));
	printl( "Created trigger \"test_trigger\" for current map \"hl_c01_a1\"" );
end