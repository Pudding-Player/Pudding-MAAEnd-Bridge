<div align="center">

<img src="https://raw.githubusercontent.com/Pudding-Player/Pudding/main/assets/brand/avatar.png" width="72" alt="Pudding" />

# Pudding MaaEnd Bridge

[Pudding](https://github.com/Pudding-Player/Pudding) 接入 MaaEnd（明日方舟：终末地）的原生桥与插件。

[![License: GPL v3](https://img.shields.io/badge/License-GPL--3.0--only-blue.svg)](LICENSE) ![Platform](https://img.shields.io/badge/Platform-Windows-0078D6.svg) ![Language](https://img.shields.io/badge/C%2B%2B%20%2F%20WinRT-blue.svg)

[Pudding 主仓](https://github.com/Pudding-Player/Pudding) · [MaaEnd](https://github.com/MaaEnd/MaaEnd) · [插件说明](plugins/maaend/README.md)

</div>

> 本仓库是 Pudding 的适配桥，不是 MaaEnd 本体；MaaEnd 由其原项目维护。

## 能力

- **C++ 原生桥**：本地 HTTP 服务接收 Pudding 的调用，转发到 MaaEnd 的任务接口并回传结果。
- **插件**（`plugins/maaend/`）：工具清单与领域技能，Pudding 据此发现和调用 MaaEnd 的任务、选项与配置。

## 接入

本仓独立包含桥源码、插件清单、技能和生成协议，不依赖主仓源码即可编译。主仓通过锁定提交获取并组装，插件随安装包内置、默认停用；MaaEnd 由用户手动启动，桥不自动启动工具或游戏。

## 构建

Windows 安装 Visual Studio 2022 C++ Build Tools 后：

```cmd
plugins\maaend\bridge\build.cmd --production
```

产物位于 `target/bridge/maaend/`。生成文件已纳入版本控制；契约或公共协议变化时由主仓的 `scripts/generate-adapter-disclosure.mjs` 同步，并将结果提交本仓。

桥协议、连接边界与使用说明见[插件说明](plugins/maaend/README.md)。

## 许可

本仓库以 [GNU GPL v3.0（GPL-3.0-only）](LICENSE) 发布，Copyright © 2026 Pudding contributors。MaaEnd 与第三方依赖分别遵循其原许可与资源使用条件。
