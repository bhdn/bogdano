# example usage of one lua scriptlet inside a spec

# notice the most importat thing is the fact there's one bug in rpm or lua
# that forces me to not call "print" more than once, so I have to keep all
# lines in a table before dumping it

Name: bla
Version: 1.0
Release: 1
Group: Foo/bar
License: GPL
Summary: bli
Buildroot: %_tmpdir/%name

%{lua:
lines = {}
for i, n in posix.dir() do
	if string.sub(n, -5) == "patch" then
		table.insert(lines, "Patch"..(tostring(i))..": "..n.."\n")
	end
end
print(table.concat(lines))
}

