import qbs 1.0

Product {
    property bool isArm: qbs.architecture == "arm"
    property string HOME:"/home/danila/raspberrypi/"
    Properties {
        condition: qbs.profile == "arm"
        cpp.includePaths: ["include",
            HOME+"/usr/include"]
        cpp.libraryPaths: HOME+"/usr/lib"
        //cpp.dynamicLibraries: [ "pthread", "asound" ]
    }
    Properties {
        condition: qbs.profile == "rpi"
        cpp.includePaths: "include"
    }
    cpp.includePaths: "include"
    Depends { name: "cpp" }
}
