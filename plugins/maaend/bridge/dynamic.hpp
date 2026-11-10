// 在 maaend_bridge_core.h 的 maaend 命名空间内、领域契约定义后引入。
// Rust 宿主通过已有公开 HTTP 契约补足，不伪造私有对象反射。
#pragma once
namespace dynamic {
namespace d = pudding::disclosure;
inline d::Array BackendMethods() {
  return d::Array::Parse(LR"JSON([
    {"name":"handle_get_adb_devices","route":"/api/maa/devices","summary":"扫描 ADB 设备并更新宿主缓存","parameters":[]},
    {"name":"handle_get_win32_windows","route":"/api/maa/windows","summary":"扫描 Win32 窗口并更新宿主缓存","parameters":[]},
    {"name":"handle_get_wlroots_sockets","route":"/api/maa/wlroots-sockets","summary":"扫描 wlroots socket 并更新宿主缓存","parameters":[]},
    {"name":"handle_get_gamescope_instances","route":"/api/maa/gamescope-instances","summary":"扫描 gamescope 实例并更新宿主缓存","parameters":[]},
    {"name":"handle_connect_controller","route":"/api/maa/instances/:id/connect","summary":"为现有实例连接现场已发现的 Win32 窗口；仅提交，不证明连接完成","parameters":["instanceId","handle","screencapMethod","mouseMethod","keyboardMethod"]}
  ])JSON");
}
inline d::Object BackendInspect(const d::Object& args) {
  if (!d::GapRequest(args)) return d::GapError();
  auto gap = d::Text(args, L"gapId"); if (!gap.empty()) (void)d::LoadReceipt(gap, "gap");
  auto offset = d::Page(args, L"offset", 0, 1000000), limit = d::Page(args, L"limit", 20, 50);
  if (!limit) return d::Error("invalid_request", "limit 必须大于零");
  std::vector<d::Object> matches;
  for (auto method : BackendMethods()) {
    auto object = method.GetObject(); auto query = d::Text(args, L"query");
    if (query.empty() || (d::Text(object, L"name") + d::Text(object, L"summary")).find(query) != std::string::npos) matches.push_back(object);
  }
  d::Array members; auto end = (std::min)(matches.size(), static_cast<size_t>(offset) + limit);
  for (size_t i = offset; i < end; ++i) {
    auto member = d::Object::Parse(matches[i].Stringify());
    d::Text(member, L"source", "MXU:web_server.rs@d556cce"); d::Flag(member, L"runtimeVerified", false);
    if (!gap.empty()) {
      d::Object candidate; d::Text(candidate, L"target", "backend"); d::Text(candidate, L"member", d::Text(member, L"name"));
      d::Text(candidate, L"gapId", gap); candidate.Insert(L"declaration", matches[i]);
      d::Text(member, L"memberId", d::StoreReceipt("member", candidate)); d::Flag(member, L"canPrepareInvoke", true);
    }
    members.Append(member);
  }
  d::Object out; out.Insert(L"members", members); out.Insert(L"total", d::Value::CreateNumberValue(static_cast<double>(matches.size())));
  if (end < matches.size()) out.Insert(L"nextOffset", d::Value::CreateNumberValue(static_cast<double>(end)));
  else out.Insert(L"nextOffset", d::Value::CreateNullValue());
  d::Flag(out, L"fallbackOnly", true);
  d::Text(out, L"next", "这些是已有后端公开契约，静态声明不证明当前可用。仅缺口补足时预览并执行，不透传 URL、原始流水线或外部命令。");
  return out;
}
inline bool RegularField(const d::Object& selector) {
  auto member = d::Text(selector, L"member");
  if (member == "enabled" || member == "runOnce" || member == "taskName" || member == "id" || member == "controllerName" || member == "resourceName") return true;
  for (auto part : selector.GetNamedArray(L"path", d::Array{})) {
    auto key = winrt::to_string(part.GetString());
    if (key == "optionValues") return true; // 已有选项校验不能通过原始叶字段修改绕过。
  }
  return false;
}
inline bool AllowedField(const d::Object& selector) {
  if (d::UnsafeName(d::Text(selector, L"member")) || RegularField(selector)) return false;
  for (auto part : selector.GetNamedArray(L"path", d::Array{}))
    if (part.ValueType() != json::JsonValueType::String || d::UnsafeName(winrt::to_string(part.GetString()))) return false;
  return true;
}
inline void AddFieldReceipts(d::Object& out, const d::Object& args, const SnapshotFetch& snapshot, const std::string& catalogHash) {
  auto gap = d::Text(args, L"gapId"); if (gap.empty() || out.HasKey(L"error")) return;
  (void)d::LoadReceipt(gap, "gap");
  auto path = args.GetNamedArray(L"path", d::Array{});
  for (auto v : out.GetNamedArray(L"members", d::Array{})) {
    auto member = v.GetObject(); auto name = d::Text(member, L"name");
    d::Object candidate; d::Text(candidate, L"target", "config"); d::Text(candidate, L"member", name); candidate.Insert(L"path", path);
    if (!AllowedField(candidate)) continue;
    auto full = d::Array::Parse(path.Stringify()); full.Append(d::Value::CreateStringValue(ToH(name)));
    auto current = d::JsonAt(snapshot.config, full);
    if (!d::SimpleJson(current)) continue;
    candidate.Insert(L"before", current); d::Text(candidate, L"gapId", gap);
    InsertStr(candidate, L"revision", snapshot.revision); d::Text(candidate, L"catalogHash", catalogHash);
    d::Text(member, L"memberId", d::StoreReceipt("member", candidate)); d::Flag(member, L"canPrepareSet", true);
  }
}
inline long long Integer(const d::Object& args, const wchar_t* key) {
  auto value = args.Lookup(key);
  if (value.ValueType() != json::JsonValueType::Number || !std::isfinite(value.GetNumber()) || value.GetNumber() < 0 ||
      value.GetNumber() > 9007199254740991.0 || std::floor(value.GetNumber()) != value.GetNumber()) throw std::invalid_argument("参数需要非负无损整数");
  return static_cast<long long>(value.GetNumber());
}
inline d::Object Prepare(BridgeContext& ctx, const d::Object& args) {
  if (!d::GapRequest(args)) return d::GapError();
  auto gap = d::LoadReceipt(d::Text(args, L"gapId"), "gap");
  auto candidate = d::LoadReceipt(d::Text(args, L"memberId"), "member");
  if (d::Text(candidate, L"gapId") != d::Text(args, L"gapId")) return d::Error("gap_conflict", "成员不属于本次缺口");
  if (d::Text(args, L"purpose").size() < 8) return d::Error("purpose_required", "需要本次操作的具体目的与语义依据");
  auto catalog = FetchCatalog(ctx); if (!catalog.ok) return catalog.error;
  auto target = d::Text(candidate, L"target"), operation = d::Text(args, L"operation");
  d::Object payload, preview;
  d::Text(payload, L"operation", operation); d::Text(preview, L"target", target);
  d::Text(preview, L"member", d::Text(candidate, L"member")); d::Text(preview, L"purpose", d::Text(args, L"purpose"));
  d::Text(preview, L"gapObservation", d::Text(gap, L"observation"));
  if (target == "config" && operation == "set") {
    if (!AllowedField(candidate) || !args.HasKey(L"value")) return d::Error("field_forbidden", "已有专用接口或受限字段不通过动态入口写入");
    auto snap = FetchSnapshot(ctx); if (!snap.ok) return snap.error;
    if (snap.revision != candidate.GetNamedString(L"revision") || catalog.hash != d::Text(candidate, L"catalogHash")) return d::Error("revision_conflict", "目录或配置已变化");
    auto before = candidate.Lookup(L"before"), after = args.Lookup(L"value");
    if (!d::SimpleJson(after) || before.ValueType() != after.ValueType()) return d::Error("type_conflict", "只修改已有简单字段并保持类型");
    payload.Insert(L"value", after); preview.Insert(L"before", before); preview.Insert(L"after", after);
    preview.Insert(L"path", candidate.GetNamedArray(L"path"));
    d::Text(preview, L"verification", "单叶字段变更，经官方 revision 条件保存并核对完整配置；不修改其他字段");
  } else if (target == "backend" && operation == "invoke") {
    auto keywords = args.GetNamedObject(L"keywords", d::Object{});
    if (args.GetNamedArray(L"arguments", d::Array{}).Size()) return d::Error("invalid_request", "公开 HTTP 契约使用具名参数");
    auto declaration = candidate.GetNamedObject(L"declaration"); auto names = declaration.GetNamedArray(L"parameters");
    if (keywords.Size() != names.Size()) return d::Error("invalid_request", "参数必须与后端声明完全一致");
    for (auto name : names) if (!keywords.HasKey(name.GetString())) return d::Error("invalid_request", "缺少声明参数");
    if (d::Text(candidate, L"member") == "handle_connect_controller") {
      auto id = d::Text(keywords, L"instanceId"); if (!ValidInstanceId(id)) return d::Error("invalid_request", "需要现存实例 id");
      auto state = FetchState(ctx); if (!state.ok) return state.error;
      d::Object instance; if (!InstanceExists(state, id, instance) || InstanceBool(instance, L"is_running")) return d::Error("instance_unavailable", "实例不存在或正在运行");
      auto handle = Integer(keywords, L"handle"); bool found = false; d::Array windows;
      if (TryGetArray(state.root, L"cached_win32_windows", windows)) for (auto window : windows)
        if (window.ValueType() == json::JsonValueType::Object && Integer(window.GetObject(), L"handle") == handle) {
          found = true; d::Text(candidate, L"windowHash", Sha256Hex(ToUtf8(window.Stringify())));
        }
      if (!found) return d::Error("window_not_observed", "窗口句柄未在宿主当前扫描缓存中出现");
      for (auto key : {L"screencapMethod", L"mouseMethod", L"keyboardMethod"}) (void)Integer(keywords, key);
      d::Text(candidate, L"instanceHash", Sha256Hex(ToUtf8(instance.Stringify())));
    }
    payload.Insert(L"keywords", keywords); preview.Insert(L"arguments", keywords);
    d::Text(preview, L"source", "MXU:web_server.rs@d556cce");
    d::Text(preview, L"effect", d::Text(declaration, L"summary"));
    d::Flag(preview, L"runtimeRouteVerified", false);
  } else return d::Error("operation_mismatch", "后端方法使用 invoke，已有配置叶字段使用 set");
  d::Text(candidate, L"catalogHash", catalog.hash); candidate.Insert(L"payload", payload);
  d::Object out; d::Text(out, L"planId", d::StoreReceipt("plan", candidate)); out.Insert(L"preview", preview);
  d::Flag(out, L"prepared", true); d::Flag(out, L"executed", false); out.Insert(L"expiresInMs", d::Value::CreateNumberValue(300000));
  return out;
}
inline d::Object Execute(BridgeContext& ctx, const std::string& route, const d::Object& args) {
  if (args.Size() != 1) return d::Error("invalid_request", "动态写入只接受预览票据，不接受替换参数");
  auto receipt = d::LoadReceipt(d::Text(args, L"planId"), "plan");
  auto expected = route == "/host/set" ? "set" : "invoke";
  if (d::Text(receipt.GetNamedObject(L"payload"), L"operation") != expected) return d::Error("operation_mismatch", "执行入口与预览不一致");
  (void)d::LoadReceipt(d::Text(receipt, L"gapId"), "gap");
  auto plan = d::LoadReceipt(d::Text(args, L"planId"), "plan", true);
  auto catalog = FetchCatalog(ctx); if (!catalog.ok) return catalog.error;
  if (catalog.hash != d::Text(plan, L"catalogHash")) return d::Error("catalog_conflict", "宿主声明已经变化，本次未执行");
  auto payload = plan.GetNamedObject(L"payload");
  if (expected == std::string("set")) {
    auto snap = FetchSnapshot(ctx); if (!snap.ok) return snap.error;
    if (snap.revision != plan.GetNamedString(L"revision")) return d::Error("revision_conflict", "配置已变化，本次未执行");
    if (!AllowedField(plan)) return d::Error("field_forbidden", "该字段不能使用动态写入");
    auto next = d::Object::Parse(snap.config.Stringify());
    auto path = plan.GetNamedArray(L"path"); auto full = d::Array::Parse(path.Stringify());
    full.Append(d::Value::CreateStringValue(ToH(d::Text(plan, L"member"))));
    if (!DeepEqual(d::JsonAt(snap.config, full), plan.Lookup(L"before"))) return d::Error("revision_conflict", "字段值已变化");
    d::SetJsonExisting(next, path, d::Text(plan, L"member"), payload.Lookup(L"value"));
    d::Object save; save.Insert(L"config", next); InsertStr(save, L"revision", snap.revision); InsertStrUtf8(save, L"client_id", kClientId);
    HttpExchange request; request.method = "POST"; request.path = "/api/config"; request.body = ToUtf8(save.Stringify());
    auto response = Upstream(ctx, request);
    if (response.network_error || !response.http_ok()) return d::Error("outcome_unknown", "条件保存没有获得完整成功回执，请读取配置核实，不重试");
    d::Object receipt; std::wstring revision;
    if (!ParseJsonObject(response.body, receipt) || !TryGetString(receipt, L"revision", revision) || revision.empty())
      return d::Error("outcome_unknown", "保存响应缺少预期 revision，不能确认提交结果，请回读核实");
    auto back = FetchSnapshot(ctx); bool verified = back.ok && back.revision == revision && DeepEqual(next, back.config);
    d::Object out; d::Flag(out, L"applied", true); d::Flag(out, L"verified", verified);
    if (back.ok) InsertStr(out, L"revision", back.revision);
    d::Text(out, L"persistence", "officialVersionedSave"); return out;
  }
  auto declaration = plan.GetNamedObject(L"declaration"), keywords = payload.GetNamedObject(L"keywords");
  HttpExchange request; request.method = "GET"; request.path = d::Text(declaration, L"route");
  if (d::Text(plan, L"member") == "handle_connect_controller") {
    auto id = d::Text(keywords, L"instanceId"); auto state = FetchState(ctx); if (!state.ok) return state.error;
    d::Object instance;
    if (!InstanceExists(state, id, instance) || InstanceBool(instance, L"is_running") || Sha256Hex(ToUtf8(instance.Stringify())) != d::Text(plan, L"instanceHash"))
      return d::Error("instance_changed", "实例状态已变化，本次未执行");
    bool windowUnchanged = false; d::Array windows;
    if (TryGetArray(state.root, L"cached_win32_windows", windows)) for (auto window : windows)
      if (Sha256Hex(ToUtf8(window.Stringify())) == d::Text(plan, L"windowHash")) windowUnchanged = true;
    if (!windowUnchanged) return d::Error("window_changed", "窗口扫描证据已变化，本次未执行");
    d::Object body; d::Text(body, L"type", "Win32");
    InsertInt(body, L"handle", Integer(keywords, L"handle")); InsertInt(body, L"screencap_method", Integer(keywords, L"screencapMethod"));
    InsertInt(body, L"mouse_method", Integer(keywords, L"mouseMethod")); InsertInt(body, L"keyboard_method", Integer(keywords, L"keyboardMethod"));
    request.method = "POST"; request.path = "/api/maa/instances/" + id + "/connect"; request.body = ToUtf8(body.Stringify());
  }
  auto response = Upstream(ctx, request);
  if (!response.http_ok()) return d::Error("outcome_unknown", "公开后端调用未获得完整成功回执，请核实状态，不自动重试");
  json::JsonValue result{nullptr};
  if (!json::JsonValue::TryParse(ToH(response.body), result)) return d::Error("outcome_unknown", "后端成功响应不是预期 JSON，结果未知");
  d::Object out; d::Flag(out, L"returned", true); d::Flag(out, L"executionVerified", false);
  if (d::Text(plan, L"member") == "handle_connect_controller") {
    if (result.ValueType() != json::JsonValueType::Object || !result.GetObject().HasKey(L"connId") ||
        result.GetObject().Lookup(L"connId").ValueType() != json::JsonValueType::Number || result.GetObject().GetNamedNumber(L"connId") <= 0)
      return d::Error("outcome_unknown", "连接响应缺少预期 connId，结果未知");
    d::Flag(out, L"submitted", true);
  } else {
    if (result.ValueType() != json::JsonValueType::Array) return d::Error("outcome_unknown", "设备扫描没有返回预期数组，结果未知");
    d::Array safe;
    for (auto entry : result.GetArray()) {
      if (safe.Size() >= 50) break;
      if (entry.ValueType() != json::JsonValueType::Object) continue;
      auto original = entry.GetObject(); d::Object item;
      for (auto key : {L"handle", L"class_name", L"window_name", L"name", L"address", L"display_no", L"pipewire_node_id", L"screencap_methods", L"input_methods"})
        if (original.HasKey(key) && d::SimpleJson(original.Lookup(key))) item.Insert(key, original.Lookup(key));
      safe.Append(item);
    }
    out.Insert(L"result", safe); out.Insert(L"total", d::Value::CreateNumberValue(result.GetArray().Size()));
    d::Flag(out, L"truncated", result.GetArray().Size() > 50);
  }
  d::Text(out, L"resultMeaning", "已获得后端 HTTP 成功回执，不证明异步连接或游戏任务完成；使用常规状态接口核实。");
  // 只回显已声明的设备标识，不透传路径、任意配置或凭据。
  return out;
}
} // namespace dynamic
