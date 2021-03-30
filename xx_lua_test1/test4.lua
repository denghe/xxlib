local this = ...
this:set_onUpdate(function(delta)
	print("lua print:", delta)
	this:set_n(12)
	this:set_name("asdf")
	return 0
end)
