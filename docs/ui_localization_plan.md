# UI 本地化计划

历史说明：
本文件记录的是一份较早期的实现计划，已经不再是当前的规范性设计来源。
当前本地化契约见 [`docs/LOCALIZATION_SPEC.md`](./LOCALIZATION_SPEC.md)，
pack/runtime 机制说明见 [`docs/LOCALE_PACKS.md`](./LOCALE_PACKS.md)。
尤其需要注意的是，本计划中仍保留了 `display_language` 这类历史概念，
它们已经被当前的 locale-pack 架构取代。

## 目标

为基于 LVGL 的 Trail Mate 界面增加可在运行时切换的 UI 本地化能力，支持英文与中文。

- 默认语言：英文
- 支持语言：英文 / 中文
- 语言切换入口：`Settings > System > Display Language`
- 切换行为：立即生效，无需重启

## 范围

本任务覆盖设备固件自身产生的、面向用户的 UI 文本，包括：

- 主菜单应用名称
- `modules/ui_shared` 下的共享 LVGL 页面
- `platform/esp/arduino_common/src/ui` 下的 ESP 特定 LVGL 页面
- 共享通知、提示框、模态框标签
- 设置页的分类名、项目名、枚举标签、动作标签和校验消息
- 菜单仪表盘组件
- 屏保提示与设备侧状态通知

本任务不翻译以下内容：

- 收到的用户消息
- 联系人名、节点名、呼号、频道名等用户生成内容
- `Meshtastic`、`MeshCore`、`LXMF`、`RNode` 这类协议品牌名
- README / 文档 / 发布说明

## 设计

### 1. 翻译字典

在 `modules/ui_shared` 中使用共享本地化模块，包含：

- `ui::i18n::Language`
- 持久化的当前语言状态
- 以 `ui::i18n::tr(const char* english)` 作为规范的字典查找入口

字典以现有英文源文本作为规范查找 key，并返回：

- 当语言为英文时，返回原始英文字符串
- 当语言为中文且存在翻译时，返回对应中文翻译
- 当没有找到翻译条目时，回退到原始英文字符串

这种方式在面对大量已有硬编码文本的 UI 代码库时，能够以较低风险完成本地化接入。

### 2. 持久化

将当前 UI 语言持久化到 `settings` namespace：

- key：`display_language`
- value：`0 = English`，`1 = Chinese`

### 3. 运行时刷新

切换语言时应当：

- 持久化新的语言值
- 刷新主菜单标签
- 以异步方式重建当前激活的应用页面

这样可以避免要求每一个控件都分别订阅语言变更事件。

### 4. 字体处理

当翻译后的文本包含非 ASCII 字符时，本地化标签应自动切换到 CJK 字体。

应使用共享字体辅助工具，以保证：

- ASCII 标签继续使用原有 UI 字体
- 中文标签在可用时切换到 Noto CJK 字体
- 未编译 CJK glyph 支持的板卡仍能安全回退到现有字体配置

## 实施步骤

1. 增加共享本地化模块与持久化辅助函数。
2. 增加菜单/应用刷新支持，使语言切换后立即生效。
3. 在 `Settings > System` 中加入 `Display Language`。
4. 将共享设置项标签、选项、提示和校验消息接入本地化。
5. 本地化菜单标题、仪表盘标签、共享组件、模态按钮和通知。
6. 本地化 Contacts / Chat / GPS / Tracker / PC Link / USB / SSTV / GNSS / Walkie Talkie / placeholder 页面中的固定文本。
7. 本地化屏保文本、电池通知、事件通知等 ESP 特定提示。
8. 运行格式化与 CI 等价构建，并修复回归问题。

## 验收标准

- 全新启动时英文是默认 UI 语言。
- 语言可以在 `Settings > System > Display Language` 中切换。
- 切换语言后当前页面立即更新。
- 返回主菜单后可看到已本地化的应用名称。
- `Back`、`Save`、`Cancel`、`Loading...`、系统 toast、页面标题等共享提示已完成本地化。
- 在支持目标上，中文文本能以 CJK glyph 覆盖正常显示。
- 现有 PlatformIO CI 构建仍能通过。
