# UI/UX Specification Index

`docs/uiux` 按“对象类型”而不是按文件历史堆叠组织。

当前目录约束如下：

- `foundation/`
  - 放全局风格、视觉语言、跨页面共享的设计基线。
  - 这里的文档不能偷带某个具体页面的布局细节。
- `pages/`
  - 放页面级规格。
  - 每个文件只解释一个页面对象，不解释可复用组件本体。
- `components/`
  - 放组件级规格与组件实现规格。
  - 这里只定义组件边界、职责、状态与实现约束，不反向定义页面。

当前文件归属：

- `foundation/firmware_visual_style.md`
- `pages/node_info_page.md`
- `pages/node_info_page_layer_popup_addendum.md`
- `components/shared_map_viewport.md`
- `components/shared_map_viewport_impl.md`
- `components/shared_map_viewport_layer_popup_addendum.md`

如果后续新增文档，必须先判断它属于哪一类对象，再落目录：

1. 它描述的是全局视觉语言，还是某个页面，还是某个组件。
2. 如果它同时想定义页面和组件，说明文档边界还没切开，应先拆分。
3. 不允许继续把页面规格、组件规格、全局风格规格平铺在同一层目录。
