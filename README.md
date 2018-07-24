# ServerWithPyhsX

Server Side Physical Simulation

# Background

本次技术调研的目的主要是为了以后做Multi-player Online Game提供技术基础,服务器端物理模拟主要可以用于实现 MMOG 状态同步模型,这种同步模型，在端游时代就使用的非常广泛，特别是MMORPG里面。它的主要实现要点是：服务器负责计算全部的游戏逻辑，并且广播这些计算的结果，客户端仅仅负责发送玩家的操作，以及表现收到的游戏结果。一般来说，玩家发送一个操作到服务器上，服务器根据玩家操作去修改内存中的游戏世界模型，同时运算游戏世界对这个操作的反应，然后把这些反应都广播给相关的多个客户端，每个客户端负责把这些数据表现出来给玩家看。这种法的优点是非常安全，由于整个游戏逻辑都在服务器上，服务器只接受合法的玩家操作，一切都经过既定逻辑的运算。另外一个优点是游戏的逻辑更新很方便，因为主要逻辑都在服务器端。一般的游戏玩法需要更新，游戏开发团队自己更新重启服务器就可以了，无需让千万个手机去下载更新包。但是这种做法的缺点也很明显，首先就是用户的体验非常依赖网络质量，如果一个用户的网速慢，其他玩家都会发现他在游戏中明显的变卡。另外一个缺点就是服务器负责了太多的游戏逻辑运算。在动作游戏里，服务器往往需要针对二维或者三维空间进行运算。最后，使用这种同步方案，由于每个游戏表现都要以数据包发往客户端，所以当一起玩的用户数量较多，这种广播的数据包量就会非常大。因此根据以上的特点，这种模型一般会在那些同局游戏人数不太多，但讲求玩法变化快和安全性高的游戏中采用这种同步方案。

# Gaol

通过制作好的Client端的游戏场景后,通过我们改写的的工具导出游戏场景的物理模型(大小,位置,角度,碰撞体),在服务器端生成一份一模一样的物理世界，从而由权威服务器去计算物理，诸如子弹有没有击中玩家等等。
 
# Tips

Game Engine:Unity3D

Physical Engine:PhysX

Serializing Tools:ProtoBuf

Develop Debug Tools:PhysX Visual Debugger

本次调研主要针对 Unity3D 进行.为了让服务器端的物理引擎和游戏客户端兼容性更强,我们采用了 Unity 自身集成的物理引擎 Physx,值得一提的是 Physx 引擎也被 Unreal 引擎采用(Unity、Unreal可以称得上是游戏开发领域使用者最多，普适性最好的两款引擎),如果读者您将采用 Unreal 引擎进行游戏的开发,这份技术调查也许也会帮到你.此外,调研过程中发现了一个将内存中物理模型图形化渲染,数值显示的 Debug Tools:PVD,可以配合使用.

PhysX支持平台和引擎如下：
 
硬件平台：Windows, OSX, Linux, XBOX, PlayStation, Andriod, IOS
 
游戏引擎：Unreal 3, Unreal 4, Unity

# Environment

System:OSX10.10.5

Unity3D:5.5.1

PhysX SDK:3.3.4

ProtoBuf:3.2.0

PhysX Visual Debugger:3.0

#Preparation

### Unity3D

如果您不关心如何将 Unity 中的场景物理信息导出到服务端,可以不下载 Unity.

### PhysXSDK
如果您不关心具体的编译过程可以直接从 Git 中下载

获取 GitHub 上 PhysX  源代码的权限,PhysX源码并不对所有人开放，需要向Repo的holder申请，操作步骤参考如下:
#### 详细步骤

注册为nvidia的developer programs

申请权限，在最下面填上自己的GitHub Username就可以了。

几分钟后查看github绑定的邮箱，就可以看到通过申请的回复邮件了

访问Github上PhysX的Repo， 就可以fork,clone下来了

#### 编译

源码clone下来之后，找到 /PhysX-3.3/PhysXSDK/Source/compiler/目录下所需要的平台(本文以linux64为例),编译出对应平台下的静态和动态链接库。这里有几个坑也和大家分享一下:
如果要配合 PVD(PhysX Visual Debugger)使用的话需要 SDK配置为 Debug 或者 Profile 级别,所以如果要使用 PVD 的话要用 make debug 编译出 Debug 版的 SDK.
因为代码 mk 文件配置为将 warning级别的提醒也视为 Error 处理所以会编译不出来,可以通过 ls *.mk|xargs sed -i 1 "s/-Werror//g"  将所有 .mk文件中的-Werror 配置删掉
集成/PhysX-3.3/Include 和 /PhysX-3.3/Lib 和 /PhysX-3.3/Bin(本文不涉及该目录的文件)到自己的程序使用。
更详细的信息可以参考根目录下的readme_osx

### ProtoBuf
brew install protobuf
protoc --version

### PhysX Visual Debugger
https://developer.nvidia.com/physx-visual-debugger

# Export Physical Scene Setting

Unity中基本的Collider分为：Box Collider, Sphere Collider, Capsule Collider,以及针对精度要求较高的Mesh Collider，还有其它的一些特定应用场景下会用到的诸如Wheel Collider，Terrain Collider等等。
我们今天主要目的搭建好整个流程，只用基本类型的Collider作为例子。
我们前后台都统一使用Google Proto buffers定义场景导出的格式，以二进制格式序列化和反序列化场景。

### Data Structure

见项目目录/src/Scene.proto

### Usage

整个Unity Editor的代码(c#) 和 依赖的第三方库(proto-net)，都已经打包好了.

下载下来直接放到Unity的Assets目录下，刷新unity.

在菜单栏中： Tools->SceneExtractor->ExtractColliders

导出当前打开的场景为一份物理配置给后台使用。

### Tips

以后可能会因为工程需要改写 Proto 结构,步奏如下:

进入Editor/BuildProtocol/Protocal

修改Scene.proto, 然后运行 Editor/BuildProtocol/build.bat，这样protobuf会在 Editor/ProtoGeneratedScrips 目录下生成新的Scene.cs

然后刷新Unity，修改 Editor/SceneExtractor.cs 脚本使用新的Scene.proto的字段

# Import Physical Setting

### Compile Background Project

本例后台项目为 C++项目以 cmake的形式创建,如果您只想看看效果的话可执行 Git 中的ServerWithPyhsX/cmake-build-debug/ServerWithPyhsX 文件,如果您想编译运行的话,可能需要稍微修改如下命令中的目录参数:

cmake --build /Users/CN40592-N/Workspace/CPP/ServerWithPyhsX/cmake-build-debug --target all -- -j 8

如果您想以 IDE 打开项目进行查看,可能需要根据ServerWithPyhsX/CMakeLists.txt进行一定的操作...

### Usage

本例使用了 OpenGL以及GLUT的framework 的库粗略的渲染了物理模型的内容,不过如果更加详细的查看物理模型中的内容推荐使用 PVD.

有了从 Unity 中导出的 XXX.scene 文件之后,将其复制到ServerWithPyhsX/scenes/中,并修改ServerWithPyhsX/src/physics_scene_manager.cpp 中的如下字段,就可以在后台复现 Unity 中的物理世界.
file_path = "../Scenes/snake5.scene";

### Code Structure

PhysicsSceneManager是核心的类，管理了初始化PhysX，导入场景等核心功能

PhysicsSceneRender和ServerCooperationRender负责渲染

PhysicsSceneCamera是控制相机镜头的类

Main.cpp是工程入口，通过RENDERFORTEST宏去控制是否开启OpenGL渲染

打开PhysicsSceneManager.cpp可以看到最重要的两个函数： InitiPhysics()和InitiSceneFromFile(),需要注意的是m_foundation， m_physics是在程序一启动的时候就必须初始化的,其它的代码配合PhysX的Documentation看，很容易就看明白了.

# Connect PhysX Visual Debugger

PVD 可以通过 TCP 或者file 的形式连接物理引擎,本例采用 TCP 的形式进行

### Code



    void PhysicsSceneManager::SetupPvdDebug()
    {
       // check if PvdConnection manager is available on this platform
     if (!m_physics->getPvdConnectionManager())
       {
          std::cout << "Warning: PhysX Visual Debugger not found running!\n";
     return;
     }
    
       const char* pvdHostIP = "127.0.0.1";
     int port = 5425;
     unsigned int timeout = 100;
     physx::PxVisualDebuggerConnectionFlags flags =
             physx::PxVisualDebuggerConnectionFlag::eDEBUG
     | physx::PxVisualDebuggerConnectionFlag::ePROFILE
     | physx::PxVisualDebuggerConnectionFlag::eMEMORY;
    
    // Create connection with PhysX Visual Debugger
     physx::debugger::comm::PvdConnection* conn = physx::PxVisualDebuggerExt::createConnection(
             m_physics->getPvdConnectionManager(),
     pvdHostIP,
     port,
     timeout,
     flags);
     if (conn)
       {
          std::cout << "Connected to PhysX Visual Debugger!\n";
     conn->createRenderer();
     }
    }


### Tips

注意:要先打开 PVD 的程序再运行我们的后台程序

如果您有兴趣更加深入的了解 PVD 可以参考http://docs.nvidia.com/gameworks/content/gameworkslibrary/physx/apexsdk/APEX_Framework/APEX_PG_PVD.html

