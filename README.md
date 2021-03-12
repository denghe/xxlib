# xxlib
base library ( runtime &amp; utils for cpp, c#, lua ... )




apk .so 减肥/去特征经验集  
1. build.gradle  
位置：android.defaultConfig.externalNativeBuild.cmake.cppFlags  
附加 -fvisibility=hidden  
效果：隐藏部分函数名  

2. CMakeLists.txt  
target_link_libraries  
附加 -Wl,--gc-sections	进一步的，使用 -Wl,-version-script -Wl,白名单.ver.  
效果：去掉未使用的或不需要导出的函数( 理论上讲有 J 字头的就够了 )  

3. 所有参与直接编译的源码（包括第三方库）  
替换 __FILE__ 为 "" 啥的  
效果：避免 so 含有 C:\xxxx\xxxx 等明确路径特征  

4. 很多代码内嵌明文字符串（例如具体的资源名 fishs/xiaochouyu.png ）  
替换为 函数 + 枚举，类似多语言方式，去查表（后期下载或从加密zip解出）  
不方便查表的，可 base64 现解存变量再用  
效果：去特征或保护  

5. 除开 so 以外的所有 res 文件，最好 zip 带密码压两层，运行时再解到可写目录  
效果：避免跨 apk 包文件列表雷同  

6. 指定 某些 cpp 独特编译参数  
set_source_files_properties(Classes/test.cpp PROPERTIES COMPILE_FLAGS "-fvisibility=hidden -fno-rtti")  
效果：避免暴露为 typeid 服务的类名  
注意：这种文件只能暴露 c 接口，且不可直接 include 别的开启了 rtti 的 .h  

7. 具体到 cocos  
	a. cocos\base\ccConfig.h 里面用不到的特性，宏值改 0  
	b. 将用不到的 第三方库 目录改名，再通过编译报错，去 cmake 等地方大段注释( 有时 cmake 里看似用来开关组件的变量，不起作用 )  
	c. java 部分内嵌字串要求同上，别的类名 函数名可混淆( cocos engine 常见部分可以不管，自己附加部分要特别注意)  
	d. AndroidManifest.xml 内容重组 防止和别的包完全一致  

附录： so 特征分析手段  
zip 解开 apk 在 lib 找到 so  
1. 用 strings 导出 特征字符串列表( 混合了导出函数列表 和 代码内写死的字串 )  
2. 用 IDA 打开 看 Exports, Strings( 可排序，更方便 ), 配合插件甚至可以删掉各种导出函数，只留下 JNI Java 字样的.  
