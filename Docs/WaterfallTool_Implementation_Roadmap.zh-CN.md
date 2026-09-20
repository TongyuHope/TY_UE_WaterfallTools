# TYWaterfallTools 实施路线图

> 当前工程：UE 5.8  
> 当前插件：`TYWaterfallTools`  
> 学习参考：`SHADERSOURCE Waterfall Tool 2`（目标插件声明为 UE 5.7）  
> 文档目标：作为后续开发的唯一入口，记录架构、阶段、文件映射、验收标准和进度。

## 1. 使用方式

每次继续开发时，先执行以下流程：

1. 阅读本文档的“当前状态”和当前阶段。
2. 运行 `git status --short`，确认工作区变化。
3. 只阅读当前阶段列出的本项目文件和参考文件。
4. 一次只完成一个可验证的小任务。
5. 编译并在编辑器中完成对应验收。
6. 更新本文档中的复选框、决策记录和已知问题。
7. 单独提交该阶段，避免把多个功能混在一个提交中。

后续开发不需要再次遍历整个参考插件。只有当当前阶段的说明不足时，才读取该阶段列出的参考源码。

## 2. 当前状态

### 已有内容

- [x] 插件基础目录和 `.uplugin`
- [x] UE Editor Mode 模板代码
- [x] Editor Mode Toolkit、Commands 和示例 Interactive Tool
- [x] Git 仓库初始化
- [x] Runtime 模块
- [ ] 瀑布 Actor
- [ ] 路径模拟
- [ ] 动态网格（阶段 4 已实现并通过编译，等待编辑器验收）
- [ ] 自定义瀑布编辑界面
- [ ] Niagara / Audio
- [ ] 静态网格烘焙

### 当前插件限制

当前 `TYWaterfallTools.uplugin` 只有一个 `Editor` 模块。因此：

- 该模块中的 Actor 和 Component 不能作为正常运行时功能打包。
- 编辑器 UI、生成算法和最终运行时数据会混在一起。
- 后续功能越多，编译依赖和维护成本越高。

第一阶段必须先增加 Runtime 模块。保留现有 `TYWaterfallTools` 作为 Editor 模块，可以避免立即重命名现有模板文件。

## 3. 目标范围

### 第一版必须实现

- 在关卡中放置 `ATYWaterfallActor`。
- 使用顶部样条定义瀑布的宽度、起点和初始方向。
- 从顶部样条生成多条水流路径。
- 路径受重力、初速度、阻力和场景碰撞影响。
- 根据路径生成可预览的动态带状网格。
- 在自定义 Editor Mode 中生成、清除和重新生成。
- 支持材质参数需要的 UV 和顶点颜色数据。
- 能将动态网格烘焙为静态网格资产。

### 第一版暂不追求

- 真实流体模拟。
- 完全复刻参考插件的全部 UI。
- 一开始就实现四种网格模式。
- 一开始就支持每个路径点放置任意 FX。
- 与参考插件完全相同的参数、算法和资源。

### 版权边界

参考插件源码带有版权声明。本项目学习其系统边界和功能思路，但应使用自己的：

- 类名和模块名；
- 算法实现；
- Slate 布局；
- 材质、Niagara、图标和其他内容资源；
- 文档和用户文案。

不要直接复制参考插件的大段实现或内容资源。

## 4. 推荐架构

```text
TYWaterfallTools.uplugin
├── TYWaterfallToolsRuntime       Type=Runtime
│   ├── Actors
│   │   └── ATYWaterfallActor
│   ├── Components
│   │   ├── UTYWaterfallPathComponent
│   │   ├── UTYWaterfallMeshComponent
│   │   ├── UTYWaterfallVFXComponent
│   │   └── UTYWaterfallSettingsComponent
│   ├── Generation
│   │   ├── FTYWaterfallPathBuilder
│   │   ├── FTYWaterfallMeshBuilder
│   │   └── FTYWaterfallBakeBuilder
│   └── Data
│       ├── FTYWaterfallSimPoint
│       ├── FTYWaterfallSample
│       └── ETYWaterfallPointState
└── TYWaterfallTools              Type=Editor（保留现有模块名）
    ├── EditorMode
    ├── Details
    ├── Slate
    ├── Visualizers
    └── Commands
```

### 数据流

```text
Editor Mode / Details Panel
            |
            v
UTYWaterfallSettingsComponent
            |
            v
FTYWaterfallPathBuilder
            |
            v
UTYWaterfallPathComponent[]
            |
            v
FTYWaterfallMeshBuilder
            |
            v
UTYWaterfallMeshComponent
        |                 |
        v                 v
Niagara / Audio     Static Mesh Bake
```

### 核心职责

`ATYWaterfallActor`

- 拥有所有默认组件。
- 提供路径、网格和 FX 的查询接口。
- 协调 Builder，但不直接实现复杂算法。
- 编辑器生成时允许 Tick；正常运行时默认不 Tick。

`UTYWaterfallSettingsComponent`

- 保存用户可编辑参数。
- 验证参数范围。
- 向 UI 提供生成、取消和清理入口。
- 不直接编写 Slate UI。

`UTYWaterfallPathComponent`

- 保存一条模拟路径及其采样缓存。
- 负责单条路径的模拟、重采样和查询。
- 默认关闭组件 Tick，由 Builder 统一调度。

`FTYWaterfallPathBuilder`

- 创建、排队和分帧处理路径。
- 管理开始、取消、成功和失败状态。
- 避免一次生成阻塞编辑器。

`FTYWaterfallMeshBuilder`

- 消费路径采样数据。
- 构建顶点、三角形、法线、UV 和顶点颜色。
- 第一版只实现 Per-Path 带状网格。

`TYWaterfallTools` Editor 模块

- Editor Mode、Toolkit、菜单、Details 定制和可视化器。
- 不承担运行时数据和核心生成算法。

## 5. 阶段路线图

## 阶段 0：建立基线和模块边界

### 目标

在不改变现有 Editor Mode 行为的前提下，增加 Runtime 模块，并确保插件可以编译和加载。

### 实施步骤

- [x] 在 `.uplugin` 中启用 `CanContainContent`。
- [x] 添加 `TYWaterfallToolsRuntime` Runtime 模块。
- [x] 保留现有 `TYWaterfallTools` 为 Editor 模块。
- [x] Editor 模块依赖 `TYWaterfallToolsRuntime`。
- [x] Runtime 模块初始只依赖 `Core`、`CoreUObject` 和 `Engine`。
- [x] 确认 Editor 模块不会被非 Editor Target 加载。
- [x] 添加 Runtime 模块启动/关闭日志。
- [x] 完成一次 Development Editor 编译。

### 预计文件

```text
TYWaterfallTools.uplugin
Source/TYWaterfallTools/TYWaterfallTools.Build.cs
Source/TYWaterfallToolsRuntime/TYWaterfallToolsRuntime.Build.cs
Source/TYWaterfallToolsRuntime/Public/TYWaterfallToolsRuntimeModule.h
Source/TYWaterfallToolsRuntime/Private/TYWaterfallToolsRuntimeModule.cpp
```

### 验收标准

- 插件可以在 UE 5.8 编辑器中启用。
- 现有 Editor Mode 仍然出现。
- Output Log 中没有模块加载错误。
- Runtime 模块可以被游戏模块引用。

## 阶段 1：瀑布 Actor 骨架

### 目标

创建可放入关卡的 Actor，建立后续路径、网格和 FX 的组件所有权。

### 实施步骤

- [x] 创建 `ATYWaterfallActor`。
- [x] 添加 `USceneComponent` 根组件。
- [x] 添加顶部 `USplineComponent`。
- [x] 设置两个默认样条点，形成可编辑的瀑布顶部宽度。
- [x] 添加用于最终烘焙结果的 `UStaticMeshComponent`。
- [x] 添加 Editor-only 的终止平面可视化组件。
- [x] 使用 `WITH_EDITORONLY_DATA` 隔离纯编辑器组件。
- [x] 默认关闭运行时 Tick。
- [x] 在编辑器中验证移动、旋转、复制和保存 Actor。

### 建议接口

```cpp
UCLASS()
class TYWATERFALLTOOLSRUNTIME_API ATYWaterfallActor : public AActor
{
    GENERATED_BODY()

public:
    ATYWaterfallActor();

    USplineComponent* GetTopSpline() const;
    UStaticMeshComponent* GetBakedMeshComponent() const;
};
```

### UObject 约束

- 所有持有的 `UObject*` / Component 指针必须通过 `UPROPERTY` 跟踪。
- 对外默认返回只读访问，避免任意对象改写内部状态。
- 只有确实需要蓝图修改的属性才使用 `BlueprintReadWrite`。

### 验收标准

- Actor 能从 Place Actors 或 Content Browser 放入关卡。
- 顶部样条可通过视口编辑。
- 保存并重开关卡后组件状态不丢失。
- PIE 时 Actor 不执行无意义 Tick。

## 阶段 2：单条路径模拟 MVP

### 目标

从顶部样条的中点生成一条路径，先证明基础运动和碰撞链路正确。

### 数据类型

```cpp
UENUM()
enum class ETYWaterfallPointState : uint8
{
    Start,
    Airborne,
    Sliding,
    Stopped,
    Terminated
};

USTRUCT()
struct FTYWaterfallSimPoint
{
    GENERATED_BODY()

    UPROPERTY()
    FVector Position = FVector::ZeroVector;

    UPROPERTY()
    FVector Velocity = FVector::ZeroVector;

    UPROPERTY()
    FVector HitNormal = FVector::UpVector;

    UPROPERTY()
    ETYWaterfallPointState State = ETYWaterfallPointState::Start;
};
```

### 实施步骤

- [x] 创建 `UTYWaterfallPathComponent : USplineComponent`。
- [x] 禁用 Path Component 自身 Tick。
- [x] 添加模拟点数组。
- [x] 从 Top Spline 中点取得位置和方向。
- [x] 实现初速度、重力和线性阻力。
- [x] 使用固定时间步长，保证结果可复现。
- [x] 使用 Sweep/Line Trace 检查场景碰撞。
- [x] 碰撞后将速度投影到碰撞平面，实现基础滑动。
- [x] 到达终止高度或最大步数时结束。
- [x] 将模拟点写入 Spline Points。
- [x] 实现 Clear Path。

### 算法基线

第一版使用半隐式欧拉或中点积分即可：

```text
Velocity += Gravity * DeltaTime
Velocity *= DragFactor
NextPosition = Position + Velocity * DeltaTime
Trace(Position, NextPosition)
```

先保证行为稳定和容易调试，不提前实现路径间排斥、湍流传播或搜索树。

### 验收标准

- 点击生成后出现一条可见 Spline。
- 无碰撞时路径受重力下落。
- 遇到斜面时沿表面滑动。
- 使用相同参数重复生成结果一致。
- 最大步数可以防止无限模拟。

## 阶段 3：多路径和分帧任务

### 目标

沿 Top Spline 生成多条路径，并将计算分散到编辑器多帧。

### 实施步骤

- [x] 创建 `UTYWaterfallSettingsComponent`。
- [x] 添加 `NumPaths`、`InitialSpeed`、`Gravity`、`Drag`、`FixedDeltaTime`、`MaxSteps`。
- [x] 添加 Seed 和 `FRandomStream`，禁止核心生成使用全局 `FMath::Rand`。
- [x] 创建 `FTYWaterfallPathBuilder`。
- [x] 将任务状态定义为 Idle、Generating、Cancelling、Completed、Failed。
- [x] 按 Top Spline 参数均匀创建多个 Path Component。
- [x] 每帧限制模拟预算，避免冻结编辑器。
- [x] 添加取消操作。
- [x] 添加 Undo/Redo 的 `FScopedTransaction`。
- [x] 完成生成后调用 `Modify()`、`MarkPackageDirty()`。
- [x] 删除路径时同步清理动态网格和 FX（当前尚无网格/FX，后续 Builder 将接入同一清理入口）。

### 生命周期要求

- Builder 若是普通 `F` 结构体，不拥有 UObject 生命周期。
- Builder 内部不要长期保存未经跟踪且可能失效的裸 UObject 指针。
- 跨帧引用优先使用 `TWeakObjectPtr`，每次使用前检查 `IsValid()`。
- Editor Mode 退出、Actor 删除或地图切换时必须取消任务。

### 验收标准

- [x] 可以生成 1、8、32 条路径。
- [x] 生成期间编辑器视口仍可响应。
- [x] Cancel 后不会留下半注册组件。
- [x] Undo 可以撤销生成，Redo 可以恢复或重新执行预期状态。

## 阶段 4：Per-Path 动态带状网格

### 目标

把每条路径转换为最简单、最容易验证的带状动态网格。

### 实施步骤

- [x] 采用底层 `GeometryFramework` / `GeometryCore`，本阶段无需启用高层 `GeometryScripting` 插件。
- [x] Runtime 模块添加 `GeometryFramework` 和 `GeometryCore` 依赖。
- [x] 创建 `UTYWaterfallMeshComponent : UDynamicMeshComponent`。
- [x] 沿每条路径按距离均匀采样。
- [x] 计算每个采样点的切线和横向向量。
- [x] 每个采样点生成左、右两个顶点。
- [x] 相邻采样点生成两个三角形。
- [x] 生成法线和 UV0，由 Dynamic Mesh 自动计算切线。
- [x] 将路径累计距离写入 UV 的 V 分量。
- [x] 支持 Ribbon Width、Mesh Sample Spacing 和 Mesh UV Length 参数。
- [x] 实现 Clear Dynamic Mesh，并在重新生成/清除路径时同步清理网格。

### 三角形连接

```text
L0 ---- L1
|    /  |
|  /    |
R0 ---- R1

Triangle A: L0, L1, R0
Triangle B: R0, L1, R1
```

使用 Unreal 的顺时针正面绕序，使单面材质从瀑布上方可见；法线同样朝向水面上方。

### 验收标准

- 每条路径都生成连续网格。
- 网格没有明显断裂、翻面和 NaN 顶点。
- 调整宽度不会改变路径中心。
- 材质 UV 沿水流方向连续。
- 清理路径时网格同步清理。

## 阶段 5：采样数据和材质通道

### 目标

建立稳定的路径采样数据模型，为材质和 Niagara 提供输入。

### 建议采样数据

```cpp
USTRUCT()
struct FTYWaterfallSample
{
    GENERATED_BODY()

    FVector Position = FVector::ZeroVector;
    FVector Tangent = FVector::ForwardVector;
    FVector Normal = FVector::UpVector;
    FVector Velocity = FVector::ZeroVector;
    float Distance = 0.0f;
    float NormalizedDistance = 0.0f;
    float Speed = 0.0f;
    float Impact = 0.0f;
    float Turbulence = 0.0f;
    float RandomValue = 0.0f;
};
```

### 实施步骤

- [x] 按距离重采样，不直接使用不均匀的模拟点作为顶点。
- [x] 缓存 Position、Tangent、Normal、Velocity 和 Distance。
- [x] 基于碰撞冲击和方向变化计算简化 Turbulence。
- [x] 定义固定材质通道协议。
- [x] UV0 用于基础材质坐标。
- [x] UV1 保存累计距离和归一化距离。
- [x] UV2 保存速度（除以 1000）和湍流。
- [x] Vertex Color 保存湍流、冲击和稳定随机值。
- [x] 在文档中锁定通道语义，后续不随意更改。

### 阶段 5 材质通道协议

| 通道 | X / R | Y / G | Z / B | W / A |
| --- | --- | --- | --- | --- |
| UV0 | Ribbon 横向 0 到 1 | 距离 / Mesh UV Length | - | - |
| UV1 | 累计距离 | 归一化距离 0 到 1 | - | - |
| UV2 | 速度 / 1000 | 湍流 0 到 1 | - | - |
| Vertex Color | 湍流 0 到 1 | 冲击 0 到 1 | 稳定随机值 0 到 1 | 1 |

`FTYWaterfallSample` 是 mesh、材质和后续 Niagara 之间的公共数据协议。随机值由路径种子和距离生成；改变同一 Seed 下的采样间距不会改变相同距离位置的随机结果。

### 验收标准

- Debug Material 能显示距离渐变。
- Debug Material 能显示速度或湍流数据。
- 同一 Seed 的随机通道稳定。
- 重采样间距变化时材质流向仍连续。

## 阶段 6：Editor Mode 最小工作流

### 目标

让用户不依赖 Actor 默认 Details 面板即可完成核心操作。

### 实施步骤

- [ ] 精简当前模板生成的示例 Tool。
- [ ] Editor Mode 进入时识别当前选择的 `ATYWaterfallActor`。
- [ ] 未选择瀑布时显示创建/选择提示。
- [ ] 添加 Generate Paths 按钮。
- [ ] 添加 Generate Mesh 按钮。
- [ ] 添加 Clear 按钮。
- [ ] 添加 Cancel 按钮和生成进度状态。
- [ ] 生成期间禁用会破坏状态的控件。
- [ ] Actor 选择变化时刷新 Toolkit。
- [ ] Editor Mode 退出时解绑 Delegate 并取消未完成任务。

### UI 原则

- 第一版不复制参考插件的大型多 Tab UI。
- 只显示当前工作流必需的控件。
- 参数继续使用 Details View，命令放在工具栏区域。
- 所有长任务都要提供取消路径。

### 验收标准

- 选择不同瀑布 Actor 时面板正确切换。
- 删除当前 Actor 后面板不会访问失效对象。
- 生成期间按钮状态正确。
- 退出 Editor Mode 后没有残留绑定或崩溃。

## 阶段 7：Details 定制和路径可视化

### 目标

提升参数编辑、路径选择和调试效率。

### 实施步骤

- [ ] 注册 `ATYWaterfallActor` Details Customization。
- [ ] 注册 `UTYWaterfallSettingsComponent` Details Customization。
- [ ] 按 Simulation、Paths、Mesh、Material、FX、Bake 分组。
- [ ] 根据当前状态显示或隐藏相关参数。
- [ ] 注册 Path Component Visualizer。
- [ ] 在视口绘制速度、法线、碰撞点和终止点。
- [ ] 添加显示顶点和三角形的 Debug 操作。
- [ ] 在模块 Shutdown 时对称注销全部自定义项。

### 验收标准

- Details 面板刷新不丢失当前对象。
- Hot Reload 或模块关闭时不留下无效注册。
- Debug 绘制有明确开关和持续时间。
- Shipping/非 Editor 构建不包含编辑器 UI 依赖。

## 阶段 8：扩展网格模式

### 目标

在 Per-Path 稳定之后扩展其他表现形式。

### 推荐顺序

- [ ] Cross：每条路径生成两个相交带状平面。
- [ ] Splash：在路径终点生成扇形或环形水花网格。
- [ ] Singular：把多条路径连接成连续水幕。

### Singular 风险

连续水幕要求相邻路径拥有兼容的采样数量和拓扑。需要处理：

- 路径长度不同；
- 路径交叉；
- 路径提前终止；
- 相邻路径点错位；
- 三角形翻转；
- UV 在路径边界不连续。

因此 Singular 必须放到最后实现，不能作为动态网格 MVP。

### 验收标准

- 每种网格模式可以独立生成和清除。
- 材质槽和显示状态互不干扰。
- Singular 在路径长度差异较大时不会产生越界访问。

## 阶段 9：Niagara 和 Audio

### 目标

基于路径数据布置顶部、中段和底部效果。

### 实施步骤

- [ ] 定义统一的 `FTYWaterfallFXPointData`。
- [ ] 添加顶部、底部和中段数据查询接口。
- [ ] 创建 `UTYWaterfallVFXComponent : UNiagaraComponent`。
- [ ] 通过 Niagara Data Interface 或数组参数传递点数据。
- [ ] 添加顶部、中段、底部 Audio Component。
- [ ] 根据路径中点或包围盒确定 Audio 位置。
- [ ] 路径重新生成后刷新 FX 数据。
- [ ] 路径清除后清理局部 FX。

### 性能要求

- 不在每帧重新上传不变的 Niagara 数组。
- 只在路径或 FX 设置变化时刷新。
- FX 资产使用软引用，避免插件加载时强制加载大型资源。

### 验收标准

- 顶部、底部和中段效果位置正确。
- Actor 移动和旋转后局部/世界空间转换正确。
- 清除路径不会留下悬空 FX 组件。

## 阶段 10：静态网格烘焙

### 目标

将动态网格保存为可打包的 Static Mesh 资产。

### 实施步骤

- [ ] Editor 模块添加 Geometry Scripting 和 Asset Tools 相关依赖。
- [ ] 合并需要烘焙的 Dynamic Mesh。
- [ ] 让用户选择输出目录和资产名。
- [ ] 创建或覆盖 Static Mesh 前给出明确提示。
- [ ] 保存材质槽名称和材质引用。
- [ ] 设置 `BakedMeshComponent`。
- [ ] 提供动态/烘焙网格显示切换。
- [ ] 标记资产包和关卡包为 Dirty。
- [ ] 验证烘焙资产重启编辑器后仍有效。

### 验收标准

- 可以在 Content Browser 中得到 Static Mesh。
- Static Mesh 包含正确的材质槽和 UV。
- 打包游戏不依赖 Editor-only Dynamic Mesh 数据。
- 重复烘焙不会静默破坏已有资产。

## 阶段 11：测试、性能和发布

### 自动化测试候选

- [ ] 固定 Seed 生成结果一致。
- [ ] 路径数量和组件数量一致。
- [ ] 所有路径点数在合法范围内。
- [ ] 动态网格没有 NaN/Inf 顶点。
- [ ] 三角形索引不越界。
- [ ] Cancel 后任务回到 Idle。
- [ ] Actor 删除后 Builder 不继续访问对象。
- [ ] Editor 模块不会进入非 Editor Target。

### 人工测试矩阵

```text
平面自由落体
斜坡滑动
凸起和凹槽
极短 Top Spline
很宽的 Top Spline
1 / 8 / 32 / 100 条路径
很小和很大的固定时间步长
生成中取消
生成后 Undo / Redo
保存并重启编辑器
PIE
Development 打包
```

### 发布前检查

- [ ] UObject 指针均正确参与 GC。
- [ ] Delegate 在生命周期结束时解绑。
- [ ] Editor-only 代码有正确宏和模块边界。
- [ ] 没有每帧重复查找组件或 Actor。
- [ ] 没有不必要的运行时 Tick。
- [ ] 资产引用优先使用软引用。
- [ ] 模块 Startup/Shutdown 注册操作对称。
- [ ] README 包含安装和基本使用方法。
- [ ] 不包含参考插件的受版权保护资源。

## 6. 依赖规划

### Runtime 初始依赖

```text
Core
CoreUObject
Engine
```

### 动态网格阶段按需增加

```text
GeometryFramework
GeometryCore
```

只有确实调用 Geometry Script API 时再增加：

```text
GeometryScriptingCore
```

### FX 阶段按需增加

```text
Niagara
AudioMixer（仅在确有需要时）
```

### Editor 模块依赖

```text
TYWaterfallToolsRuntime
UnrealEd
LevelEditor
Slate
SlateCore
ToolMenus
PropertyEditor
EditorFramework
InteractiveToolsFramework
EditorInteractiveToolsFramework
ComponentVisualizers
AssetTools
ContentBrowser
```

依赖应按阶段增加，不要一次照搬参考插件的全部 Build.cs。

## 7. 参考源码索引

参考插件根目录：

```text
D:/UEProjectReal/VibeCoding_Test/Plugins/SHADERSO9165f204717eV6
```

| 要理解的功能 | 优先阅读文件 | 备注 |
| --- | --- | --- |
| 插件模块划分 | `Shadersource_WaterfallTool2.uplugin` | Runtime + Editor 双模块 |
| Actor 组件结构 | `Source/Shadersource_Waterfall2Runtime/Private/Actors/SH_Waterfall2.cpp` | 观察组件所有权和 Builder 协调 |
| 路径数据结构 | `Source/Shadersource_Waterfall2Runtime/Public/EditorComponents/SH_WaterfallPathComponent.h` | 模拟点、采样和缓存 |
| 单路径模拟 | `Source/Shadersource_Waterfall2Runtime/Private/EditorComponents/SH_WaterfallPathComponent.cpp` | 重力、阻力、Trace、滑动 |
| 多路径调度 | `Source/Shadersource_Waterfall2Runtime/Private/Generation/SH_WaterfallBuilder_Path.cpp` | 创建、排队、Tick、取消 |
| 动态网格 | `Source/Shadersource_Waterfall2Runtime/Private/Generation/SH_WaterfallBuilder_Mesh.cpp` | 顶点缓冲和四种模式 |
| 烘焙 | `Source/Shadersource_Waterfall2Runtime/Private/Generation/SH_WaterfallBuilder_Static.cpp` | Dynamic Mesh 到 Static Mesh |
| Editor Mode | `Source/Shadersource_Waterfall2EdMode/Private/EdMode/SH_Waterfall2EditorMode.cpp` | 进入、退出和选择变化 |
| Toolkit | `Source/Shadersource_Waterfall2EdMode/Private/EdMode/SH_Waterfall2EditorModeToolkit.cpp` | Slate 主界面和状态切换 |
| Details 定制 | `Source/Shadersource_Waterfall2EdMode/Private/Details/SH_Details_WaterfallEdit.cpp` | 参数布局和按钮 |
| 模块注册 | `Source/Shadersource_Waterfall2EdMode/Private/Shadersource_Waterfall2EdModeModule.cpp` | 菜单、命令、Details、Visualizer |
| Niagara 数据 | `Source/Shadersource_Waterfall2Runtime/Private/Components/SH_WaterfallVFXComponent.cpp` | 点数组和空间转换 |

## 8. 本项目文件索引

当前 Editor Mode 模板入口：

```text
Source/TYWaterfallTools/Private/TYWaterfallToolsModule.cpp
Source/TYWaterfallTools/Private/TYWaterfallToolsEditorMode.cpp
Source/TYWaterfallTools/Private/TYWaterfallToolsEditorModeToolkit.cpp
Source/TYWaterfallTools/Private/TYWaterfallToolsEditorModeCommands.cpp
```

模板中的 `SimpleTool` 和 `InteractiveTool` 暂时保留。阶段 6 决定它们是否仍有教学价值，再删除或替换，不在模块拆分时同时清理。

## 9. 技术决策记录

| 编号 | 决策 | 原因 | 状态 |
| --- | --- | --- | --- |
| ADR-001 | 保留 `TYWaterfallTools` 作为 Editor 模块，新增 `TYWaterfallToolsRuntime` | 减少对现有模板代码的改名和迁移 | 已决定 |
| ADR-002 | 第一种网格只实现 Per-Path Ribbon | 拓扑最简单，便于验证路径和材质数据 | 已决定 |
| ADR-003 | 生成任务分帧执行并可取消 | 防止编辑器冻结，并支持较多路径 | 已决定 |
| ADR-004 | 使用固定时间步长和 `FRandomStream` | 保证同一参数可复现 | 已决定 |
| ADR-005 | 先完成动态网格，再实现 Static Mesh Bake | 隔离几何错误和资产保存错误 | 已决定 |
| ADR-006 | 运行时 Actor 默认不 Tick | 生成是编辑器工作流，避免无意义开销 | 已决定 |

后续遇到会影响多个阶段的架构选择时，在此追加 ADR，而不是只把决定留在聊天记录中。

## 10. 进度日志模板

每完成一个小任务，在这里追加记录：

```text
### YYYY-MM-DD - 阶段 N / 任务名称

- 完成：
- 修改文件：
- 验证方式：
- 遗留问题：
- 下一步：
```

## 11. 下一步

阶段 4 已完成代码实现和 UE 5.8 编译，当前等待编辑器内的网格朝向、UV、参数和 Undo/Redo 验收。

### 2026-09-18 - 阶段 0 / Runtime 模块边界

- 完成：新增 `TYWaterfallToolsRuntime`，保留原模块作为 Editor 模块，并建立单向模块依赖。
- 修改文件：`.uplugin`、两个模块的 `Build.cs`、Runtime 模块入口文件。
- 验证方式：用户已在 UE 5.8 下完成 Development Editor 编译。
- 遗留问题：尚未进行非 Editor Target 打包验证，安排在发布测试阶段。
- 下一步：创建 `ATYWaterfallActor` 和阶段 1 所需默认组件。

### 2026-09-18 - 阶段 1 / Actor 骨架实现

- 完成：新增 `ATYWaterfallActor`，包含根组件、顶部样条、隐藏的烘焙网格组件和 Editor-only 终止平面。
- 修改文件：`Source/TYWaterfallToolsRuntime/Public/Actors/TYWaterfallActor.h`、`Source/TYWaterfallToolsRuntime/Private/Actors/TYWaterfallActor.cpp`。
- 验证方式：用户已在 UE 5.8 中重新生成工程文件并成功编译，确认 Actor 放置、样条编辑、保存和重开关卡均正常。
- 遗留问题：终止平面的可视化交互将在后续 Editor Mode 阶段完善。
- 下一步：进入阶段 2 的单条路径模拟。

### 2026-09-18 - 阶段 2 / 单条路径模拟实现

- 完成：新增 `UTYWaterfallPathComponent`、模拟点状态和 Actor 的生成/清除预览路径操作。
- 修改文件：`Source/TYWaterfallToolsRuntime/Public/Components/TYWaterfallPathComponent.h`、`Source/TYWaterfallToolsRuntime/Private/Components/TYWaterfallPathComponent.cpp`、Actor 头文件和实现文件。
- 验证方式：用户已在 UE 5.8 中完成编译，并验证生成、碰撞滑动、参数可复现和路径清理功能。
- 遗留问题：终止高度暂时以 Actor Z 为基准。
- 下一步：进入阶段 3 的多路径和分帧任务。

### 2026-09-20 - 阶段 3 / 多路径和分帧任务实现

- 完成：新增共享 Settings Component、明确的生成状态机、确定性多路径创建、每帧共享步数预算、取消和清理入口。
- 方向约定：Top Spline 定义瀑布宽度，路径使用其 Right Vector 垂直流出；`Reverse Flow Direction` 可翻转流向。
- 修改文件：Settings Component、Path Builder、Path Component 状态机、Waterfall Actor 和 Runtime Build.cs。
- 验证方式：UE 5.8 Development Editor 编译通过；用户已完成多路径、分帧生成、取消、清理、Undo/Redo 和垂直流向验证。
- 遗留问题：FX 尚未实现；动态网格已在阶段 4 接入同一清理入口。
- 下一步：通过阶段 3 验收后进入 Per-Path 动态带状网格。

### 2026-09-20 - 阶段 4 / Per-Path 动态带状网格

- 完成：新增 Dynamic Mesh Component 和 Mesh Builder，将全部有效路径合并为一个按距离采样的带状动态网格。
- 网格约定：U 为横向 0 到 1，V 为路径累计距离除以 Mesh UV Length；宽度方向由 Top Spline 切线投影到路径法平面得到。
- 参数：Ribbon Width 控制带宽，Mesh Sample Spacing 控制几何密度，Mesh UV Length 控制纵向纹理重复尺度，Waterfall Material 设置材质槽 0。
- 生命周期：重新生成、取消或清除路径时先清除派生网格；Dynamic Mesh 保留为运行时组件，路径和生成参数仍为 Editor-only 数据。
- 修改文件：Mesh Component、Mesh Builder、Settings Component、Waterfall Actor、Path Builder 和 Runtime Build.cs。
- 验证方式：UE 5.8 `UE_MCPTestEditor Win64 Development` 编译通过；编辑器视觉和交互验收待完成。
- 下一步：验证三角形正反面、UV 连续性、参数变化、清理以及 Undo/Redo，确认后提交阶段 4。

### 2026-09-20 - 阶段 5 / 采样数据和材质通道

- 完成：新增 `FTYWaterfallSample`，由路径组件根据模拟点按距离生成稳定采样缓存。
- 数据：缓存位置、切线、法线、速度、累计距离、归一化距离、速度大小、碰撞冲击、方向变化湍流和稳定随机值。
- 网格：Per-Path Ribbon 改为消费采样缓存，并写入 UV0、UV1、UV2 和 Vertex Color。
- 修改文件：`Data/TYWaterfallSample.h`、Path Component、Path Builder、Mesh Builder、Mesh Component。
- 验证方式：用户已完成编译，并使用 Debug Material 验证距离、速度、湍流、冲击和稳定随机通道正确。
- 下一步：提交阶段 5，然后进入阶段 6 的 Editor Mode 最小工作流。
