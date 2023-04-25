add_includedirs("./src")
add_languages("c++20")

target("badgerdb_main")
	add_files("./src/**.cpp")
	after_build(function(target)
		os.cp(target:targetfile(),"./src")
	end)