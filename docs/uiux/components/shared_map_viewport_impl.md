# Shared Map Viewport Implementation Specification

## 1. Scope

本文档定义“共享地图视口组件”的实现规格。

它不是为了锁死某个具体类名，而是为了把后续重构时最容易漂移的实现边界固定下来，使代码结构能够持续解释：

- 谁拥有视口状态
- 谁拥有业务状态
- 谁拥有瓦片后端
- 谁负责叠加层
- 图层切换时哪些状态保留，哪些状态刷新

本文件与 [shared_map_viewport.md](./shared_map_viewport.md) 配套使用：

- 前者定义“组件是什么”
- 本文件定义“组件应如何落地”

---

## 2. Implementation Goal

目标不是把现有 `GPS` 页代码整体搬出来，而是把当前系统中已经存在、但被页面私有代码包住的地图主流程抽成一个共享实现层。

换句话说，重构目标是：

- 页面继续表达自己的业务语义
- 地图主流程只保留一份
- 底层瓦片后端继续复用已有能力

---

## 3. Target Decomposition

共享地图能力在实现上应至少被拆成三层。

### 3.1 Layer A: Page-Neutral Viewport Facade

这一层属于共享组件的公开入口，负责：

- 创建/销毁地图视口对象
- 接收页面传入的视口模型
- 管理交互与生命周期
- 提供投影与状态查询
- 提供共享图层状态读写核心
- 宿主页内语义覆盖层

这一层不应直接持有联系人、节点详情、GPS 页面私有状态。

### 3.2 Layer B: Map Runtime / Camera State

这一层负责：

- zoom / pan
- active base layer
- contour toggle
- focus anchor
- interaction enabled flags
- viewport availability flags
- render dirty / refresh scheduling

这一层应是页面无关的运行时状态机。

### 3.3 Layer C: Platform Tile Backend

这一层负责：

- tile calculation
- tile cache
- tile object creation
- contour overlay loading
- file path resolution
- coordinate transform helpers
- LVGL object level rendering

当前 `map_tiles.*` 已经承担了大量 Layer C 职责，后续应被保留为共享地图视口的后端，而不是页面直接消费的公共页面 API。

---

## 4. Proposed Module Ownership

### 4.1 组件主入口位置

共享地图视口组件的主入口应归属于页面共享层，目标归属建议为：

- `modules/ui_shared/include/ui/widgets/map/...`
- `modules/ui_shared/src/ui/widgets/map/...`

原因是页面代码应依赖“共享组件接口”，而不是直接依赖某个具体页面或某个具体板级页面实现。

### 4.2 后端适配位置

具体瓦片/LVGL/文件系统后端继续放在平台层，建议归属于：

- `platform/esp/.../ui/widgets/map/...`

这层负责 ESP + LVGL + 本地文件系统相关实现。

### 4.3 页面使用位置

页面层只引用共享地图视口组件，不直接引用后端私有细节。

如果页面仍然直接包含并操纵 `TileContext`、`MapTile`、tile path helper 等对象，说明组件边界仍未收敛完成。

---

## 5. Public Contract

共享地图视口组件在实现上应对页面暴露以下能力类型。

### 5.1 输入模型

页面向组件输入的应是“页面意图”，而不是后端细节，至少包括：

- 地图容器尺寸或挂载父对象
- 当前地理聚焦对象
- 初始或当前 zoom
- 当前 layer selection
- contour enabled
- 是否允许拖动
- 是否允许缩放
- 缩放锚点策略
- 页面私有覆盖层模型

### 5.2 输出能力

组件向页面输出的应是“受控能力”，至少包括：

- 请求重渲染
- 坐标投影查询
- 当前视口状态快照
- 当前共享图层状态快照
- 共享图层状态修改入口
- 当前地图是否可用
- 缺图事件/一次性通知
- 页面手势回调或状态变更回调

### 5.3 不应暴露的内容

以下内容不应作为页面公开 API：

- tile record vector
- decoded image cache entry
- contour object 指针
- tile placeholder 对象
- 文件路径拼接细节

这些都属于后端内部实现。

---

## 6. UI Object Tree

共享地图视口组件内部应至少维持如下对象层次：

```text
MapViewportRoot
├─ TileLayer
├─ SemanticOverlayLayer
└─ GestureSurface
```

说明如下：

- `TileLayer` 承载基础底图与 contour 这类地图底层图像对象
- `SemanticOverlayLayer` 承载会随地图一起移动的语义对象
- `GestureSurface` 用于接收地图手势，不承载页面固定 chrome

页面固定 chrome，例如：

- Top bar
- Node ID
- 经纬度文本
- 右侧信息列
- 页面按钮

不应内置在共享地图视口内部，而应由页面放在组件外侧或上层。

---

## 7. Camera Model

### 7.1 必须存在的状态

组件运行时应至少显式持有：

- `zoom`
- `pan_x`
- `pan_y`
- `active_base_layer`
- `contour_enabled`
- `interaction_enabled`
- `drag_enabled`
- `zoom_enabled`
- `viewport_has_map_data`
- `viewport_has_visible_map_data`

### 7.1.1 Zoom Contract

共享地图视口实现必须只保留一套缩放等级契约：

- `default_zoom = 12`
- `min_zoom = 0`
- `max_zoom = 18`

如果某个页面因为缺图、弱网格、离线瓦片覆盖不足而需要选择不同首帧 zoom，它可以在这套契约内寻找“最近可用级别”，但不得私自改写最小值、最大值或默认值。

### 7.2 焦点与锚点

实现中必须区分两个概念：

- `focus object`
- `zoom anchor`

二者通常重合，但不是同义词。

例如：

- `Node Info` 页里，focus object 和 zoom anchor 都是目标节点
- `GPS` 页里，focus object 可能是当前位置，但拖动后 camera center 可以偏离 focus object

这也意味着：

- 拖动后 `camera center` 可以临时偏离 `focus object`
- 但页面如果声明“缩放锚点始终是 focus object”，那么下一次 zoom commit 时必须按该锚点重新求解 camera

### 7.3 Follow 不属于底层默认逻辑

共享地图视口不应默认内置 “follow self”。

正确实现是：

- 页面声明自己是否 follow
- 组件只执行页面给出的 camera policy

---

## 8. Render Pipeline

组件的主渲染流程应可被解释为以下顺序：

1. 页面传入当前模型。
2. 组件归一化 layer selection。
3. 组件计算地理焦点与坐标转换。
4. 组件更新 anchor / camera state。
5. 组件驱动后端计算 required tiles。
6. 后端布局可见 tiles。
7. 组件刷新地图语义覆盖层。
8. 页面固定 chrome 保持不动。

需要注意：

- 第 7 步中的语义覆盖层更新应基于统一投影能力，而不是页面自己再做一套经纬度到屏幕坐标的推导。
- 图层切换应重走 2 到 7，但不应要求页面重建。

---

## 9. Layer Switching Implementation Rules

### 9.0 共享核心与页面入口分离

实现上必须显式区分两层：

1. 图层切换共享核心
2. 页面触发入口 chrome

图层切换共享核心负责：

- `map_source` 合法值归一化
- `Contour` 开关语义
- 配置持久化
- 缺图 / 缺 SD / 缺等高线数据的一次性通知生成

页面触发入口 chrome 负责：

- 按钮放在哪里
- 如何打开弹层
- 焦点如何落到弹层按钮上

页面入口可以不同，但共享核心必须唯一。

### 9.1 基础底图切换

实现上应遵守：

1. 修改 active base layer。
2. 通知后端刷新 render options。
3. 保留当前 camera 语义状态。
4. 保留页面覆盖层模型。
5. 让语义覆盖层按新底图投影重新定位。

禁止做法：

- 切图层时直接销毁整个页面
- 切图层时把页面业务状态重置为初始值
- 切图层时丢掉 overlay host 再让页面自己重建一切

### 9.2 Contour 开关

Contour 开关应只是底图渲染选项变化。

它不应：

- 改变 focus object
- 改变 zoom
- 改变 pan
- 改变页面 overlay 数据

### 9.3 缺图处理

组件实现必须将“缺图”建模为显式状态，而不是隐藏失败。

页面消费的是：

- 当前图层是否可用
- 是否触发一次性缺图通知

而不是自己去碰文件系统判断。

### 9.4 `Node Info` 的实现约束

`Node Info` 页可以拥有自己的 `Layer` 按钮位置和弹层承载外壳，但它不得自行重新定义：

- `OSM / Terrain / Satellite` 的枚举语义
- `Contour` 的开关语义
- 图层配置写回逻辑
- 缺图提示判定

换句话说：

- `Node Info` 页允许拥有自己的入口 chrome
- `Node Info` 页不允许拥有自己的图层状态核心

---

## 10. Overlay Contract

页面语义覆盖层应通过共享视口组件提供的宿主进行渲染。

页面只负责：

- 描述要画哪些对象
- 描述它们的样式与标签
- 响应交互后是否更新模型

组件负责：

- 提供地理点到屏幕坐标投影
- 提供覆盖层挂载容器
- 在 camera 变化时触发重新定位

这意味着 `Node Info` 页中的：

- 节点点位
- 自身点位
- 连线
- 距离

都应是共享地图视口之上的页面 overlay，而不是页面自己维护的一套“伪 tile overlay”。

---

## 11. Logging Contract

为避免后续再出现“界面黑了但不知道发生了什么”的情况，共享地图视口组件必须具备统一日志前缀，建议为：

- `[MapViewport]`

至少应在以下节点打日志：

1. create / destroy
2. attach / detach parent
3. model apply
4. layer switch
5. contour toggle
6. drag begin / drag update / drag end
7. zoom request / zoom commit
8. anchor update
9. required tile summary
10. missing tile notice
11. overlay projection refresh
12. gesture enable / disable

页面日志仍可保留，但页面日志不应取代组件日志。

---

## 12. Refactor Obligations

接受本实现规格后，后续代码重构至少必须完成以下收敛：

1. `Node Info` 页面中的地图源归一化、tile 路径拼接、世界像素转换、独立 tile image 数组等逻辑必须删除。
2. `GPS` 页面中只属于共享地图主流程的能力必须从页面私有逻辑中剥离出来。
3. 坐标系转换这类地图通用能力不得继续挂在 `gps_page_map.cpp` 这种页面文件里充当事实上的共享库。
4. 页面应改为通过共享地图视口组件 API 获取投影与交互能力。

---

## 13. File Layout Baseline

后续实现落地时，推荐至少形成以下结构：

```text
modules/ui_shared/include/ui/widgets/map/
  map_viewport.h
  map_viewport_types.h
  map_viewport_overlay.h

modules/ui_shared/src/ui/widgets/map/
  map_viewport.cpp

platform/esp/.../include/ui/widgets/map/
  map_viewport_backend.h
  map_tiles.h

platform/esp/.../src/ui/widgets/map/
  map_viewport_backend.cpp
  map_tiles.cpp
```

此处是实现布局基线，不是必须逐字符照搬的文件名；但“共享入口在 `ui_shared`、平台后端在 platform 层”这一结构含义应保持稳定。

---

## 14. Summary Baseline

一句话总结：

共享地图视口组件的正确实现，不是把某个页面抽成公共代码，而是把“地图主流程”从页面业务中分离出来，让页面只保留自己的语义与覆盖层。
