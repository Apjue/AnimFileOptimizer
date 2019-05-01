-- Package AnimFileOptimizer

newoption
{
    trigger     = "package-config",
    description = "[package] Specifies the configuration of AnimFileOptimizer to be packaged (e.g. debug_x86)",
    default = "release_x64"
}

ACTION.name = "package"
ACTION.description = "Package AnimFileOptimizer into a package_<config>_<platform> folder"

ACTION.execute = function(self, root)

    local copy_folder
    copy_folder = function(source, destination)
        os.mkdir(destination)

        local folders = os.matchdirs(source .. "/*")
        for k, v in pairs(folders) do
            copy_folder(v, destination .. "/" .. path.getname(v))
        end

        local files = os.matchfiles(source .. "/*")
        for k, v in pairs(files) do
            os.copyfile(v, destination .. "/" .. path.getname(v))
        end
    end

    local package_config = _OPTIONS["package-config"]

    if (not package_config or #package_config == 0) then
        error("No configuration provided (--package-config argument)")
    else
        package_config = string.lower(package_config)

        if (package_config ~= "debug_x86" and package_config ~= "debug_x64" and package_config ~= "release_x86" and package_config ~= "release_x64") then
            error("This configuration is not allowed: " .. package_config)
        end
    end

    local config, platform = string.match(package_config, "([^_]+)_([^_]+)")
    config = string.lower(config)
    platform = string.lower(platform)


    local executableFolder = root .. "/wdirs/" .. platform
    local executableMatches = os.matchfiles(executableFolder .. "/afo-" .. config)
    local afo_executable

    if (#executableMatches == 1) then
        afo_executable = executableMatches[1]

    else
        executableMatches = os.matchfiles(executableFolder .. "/afo-" .. config .. ".exe")

        if (#executableMatches == 1) then
            afo_executable = executableMatches[1]

        else
            error("No AnimFileOptimizer executable found in " .. executableFolder)
        end
    end


    if (#os.matchdirs(root .. "/package_" .. config .. "_" .. platform) == 1) then
        os.rmdir(root .. "/package_" .. config .. "_" .. platform)
        print("Warning: Existing folder \"/package_" .. config .. "_" .. platform .. "/\" has been deleted")
    end

    os.mkdir(root .. "/package_" .. config .. "_" .. platform)
    os.mkdir(root .. "/package_" .. config .. "_" .. platform .. "/AnimFileOptimizer")

    if (config == "debug") then
        print("Copying debug libraries...")

        local linux_libs = os.matchfiles(root .. "/wdirs/" .. platform .. "/*-d.so")
        local windows_libs = os.matchfiles(root .. "/wdirs/" .. platform .. "/*-d.dll")

        for k, v in pairs(linux_libs) do
            os.copyfile(v, root .. "/package_" .. config .. "_" .. platform .. "/AnimFileOptimizer/" .. path.getname(v))
        end

        for k, v in pairs(windows_libs) do
            os.copyfile(v, root .. "/package_" .. config .. "_" .. platform .. "/AnimFileOptimizer/" .. path.getname(v))
        end

    elseif (config == "release") then
        print("Copying release libraries...")

        local all_libs_linux = os.matchfiles(root .. "/wdirs/" .. platform .. "/*.so")
        local all_libs_windows = os.matchfiles(root .. "/wdirs/" .. platform .. "/*.dll")
        local libs = {}
        local counter = 1

        for k, v in pairs(all_libs_linux) do
            if (not string.find(v, "-d")) then
                libs[counter] = v
                counter = counter + 1
            end
        end

        for k, v in pairs(all_libs_windows) do
            if (not string.find(v, "-d")) then
                libs[counter] = v
                counter = counter + 1
            end
        end

        for k, v in pairs(libs) do
            os.copyfile(v, root .. "/package_" .. config .. "_" .. platform .. "/AnimFileOptimizer/" .. path.getname(v))
        end
        
    else
        print("Didn't understand the config (\"" .. config .. "\"); Copying all libraries...")
        
        local linux_libs = os.matchfiles(root .. "/wdirs/" .. platform .. "/*.so")
        local windows_libs = os.matchfiles(root .. "/wdirs/" .. platform .. "/*.dll")

        for k, v in pairs(linux_libs) do
            os.copyfile(v, root .. "/package_" .. config .. "_" .. platform .. "/AnimFileOptimizer/" .. path.getname(v))
        end

        for k, v in pairs(windows_libs) do
            os.copyfile(v, root .. "/package_" .. config .. "_" .. platform .. "/AnimFileOptimizer/" .. path.getname(v))
        end
    end

    -- Windows libraries
    if (#os.matchfiles(root .. "/wdirs/" .. platform .. "/assimp.dll") == 1) then
        os.copyfile(root .. "/wdirs/" .. platform .. "/assimp.dll", root .. "/package_" .. config .. "_" .. platform .. "/AnimFileOptimizer/assimp.dll")
    end

    if (#os.matchfiles(root .. "/wdirs/" .. platform .. "/libsndfile-1.dll") == 1) then
        os.copyfile(root .. "/wdirs/" .. platform .. "/libsndfile-1.dll", root .. "/package_" .. config .. "_" .. platform .. "/AnimFileOptimizer/libsndfile-1.dll")
    end
    
    if (#os.matchfiles(root .. "/wdirs/" .. platform .. "/Newton.dll") == 1) then
        os.copyfile(root .. "/wdirs/" .. platform .. "/Newton.dll", root .. "/package_" .. config .. "_" .. platform .. "/AnimFileOptimizer/Newton.dll")
    end
    
    if (#os.matchfiles(root .. "/wdirs/" .. platform .. "/soft_oal.dll") == 1) then
        os.copyfile(root .. "/wdirs/" .. platform .. "/soft_oal.dll", root .. "/package_" .. config .. "_" .. platform .. "/AnimFileOptimizer/soft_oal.dll")
    end

    print("Copying executables...")
    os.copyfile(afo_executable, root .. "/package_" .. config .. "_" .. platform .. "/AnimFileOptimizer/afo-" .. config .. "-" .. platform .. path.getextension(afo_executable))
end
