add_includedirs("./src")
add_languages("c++20")



if is_os("windows") and is_plat("mingw") then
	add_cxflags("-fexec-charset=gbk") -- windows下设置编码
end

target("badgerdb_main")
	add_files("./src/**.cpp")
	after_build(function(target)
		os.cp(target:targetfile(),"./")
	end)