-- NanoSec OS Sample Nash Script
-- ==============================
-- Run with: nash hello.nsh

-- Set variables using @
@name = "NanoSec User"
@version = "2.0"

-- Print welcome message
print "Welcome to Nash Scripting Language!"
print "Hello, @name"

-- Show variable
show @version

-- Conditional using when/otherwise/end
when @version eq 2.0 do
    print "Running Nash version @version"
otherwise
    print "Unknown version"
end

-- Loop using repeat
print ""
print "Counting:"
repeat 3 times
    print "* iteration"
end

-- Run system commands
print ""
print "System info:"
run pwd
run ls

print ""
print "Script complete!"
