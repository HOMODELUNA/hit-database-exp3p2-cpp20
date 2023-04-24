add_includedirs("./src")
add_languages("c++20")

target("badgerdb_main")
	add_files("./src/**.cpp")