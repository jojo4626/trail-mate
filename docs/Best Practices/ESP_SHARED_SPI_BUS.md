# ESP Shared SPI Bus Rule

## Scope

本文档约束 ESP 平台上“多个外设共享同一条 SPI 总线”的运行时访问规则。

当前直接受此规则约束的典型设备包括：

- `T-Deck`
- `T-LoRa Pager`

在这些设备上，显示、SD、LoRa、NFC 等外设并不是各自拥有独立 SPI 控制器，而是在板级上共享同一条 SPI 总线。

---

## Rule

`shared_spi_lock` 表达的是 **共享 SPI 总线所有权**，不是“显示锁”。

它的职责是：

- 串行化共享 SPI 总线上的访问
- 防止显示刷新与 SD / LoRa / NFC 等访问并发打总线
- 作为运行时层统一的总线仲裁入口

推荐用法：

- 直接调用 `shared_spi_lock` / `shared_spi_unlock`
- 优先使用 `SharedSpiLockGuard`

---

## Naming Contract

以下命名语义已经固定：

- `shared_spi_lock`
  - 含义是“申请共享 SPI 总线所有权”
- `SharedSpiLockGuard`
  - 含义是“一个有作用域的共享 SPI 总线占用会话”

`display_spi_lock` 只允许作为 **历史兼容别名** 存在，不能再作为新代码的主命名。

原因是：

- 真实被保护的对象不是 display
- 而是 board-level shared SPI bus

如果后续新代码继续使用“display lock”语义命名，等同于重新把总线仲裁误导回显示私有概念。

---

## Boundary

本规则约束的是：

- 平台运行时
- UI 运行时
- 地图瓦片 / 轨迹 / USB MSC / SSTV 等共享 SPI 访问路径

本规则不强制板级显示驱动内部必须如何组织其私有 mutex 实现；
但只要代码已经站在“共享 SPI 访问者”位置，而不是“显示驱动私有实现”位置，就应通过共享 SPI 语义入口表达自己。
