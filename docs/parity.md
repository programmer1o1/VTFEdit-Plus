# QTFEdit Parity Checklist (vs. Windows VTFEdit Reloaded)

Last updated: 2026-01-01

This is a living checklist intended to track feature parity between:

- **Windows (legacy)**: `legacy/VTFEdit-WinForms/` (WinForms / C++/CLI, “VTFEdit Reloaded”)
- **Qt**: `VTFEdit/` (cross‑platform, QTFEdit)

It’s based on a source scan of the Windows UI strings/handlers (primarily `legacy/VTFEdit-WinForms/VTFEdit.h`, plus `legacy/VTFEdit-WinForms/VTFOptions.h`, `legacy/VTFEdit-WinForms/VTFResources.h`, `legacy/VTFEdit-WinForms/VMTCreate.h`, `legacy/VTFEdit-WinForms/BatchConvert.h`) and the current Qt implementation. Some items may still need runtime verification.

## Legend

- **[DONE]** Implemented in QTFEdit (Qt) with comparable behavior.
- **[PARTIAL]** Implemented, but missing notable behavior/UI, or workflow differs.
- **[MISSING]** Not implemented in QTFEdit (Qt).
- **[UNKNOWN]** Not confirmed (needs testing or deeper audit).

## File / Project

- **[DONE]** Create new VTF (from image import) + create options (`VTFEdit/CreateVtfDialog.*`).
- **[DONE]** Create new VMT (basic) (`VTFEdit/CreateVmtDialog.*`).
- **[DONE]** Open VTF / open VMT (`VTFEdit/MainWindow.*`).
- **[DONE]** Recent VTF/VMT lists + clear recent (`VTFEdit/MainWindow.*`).
- **[DONE]** Save VTF / Save As VTF (`VTFEdit/MainWindow.*`).
- **[DONE]** Save VMT / Save As VMT (`VTFEdit/MainWindow.*`).
- **[DONE]** Drag & drop `.vtf`, `.vmt`, and images (`VTFEdit/MainWindow.*`).
- **[DONE]** “Recent Files” parity:
  - Qt now exposes a single `Open Recent Files` menu that mixes `.vtf` + `.vmt` in MRU order, with a single clear action and a configurable limit (default **8**, max **32**) (`VTFEdit/MainWindow.*`).

## Import / Export

- **[DONE]** Export current preview to PNG (`Export PNG…`).
- **[DONE]** Export thumbnail PNG (when present).
- **[DONE]** Export image “as” (via Qt image writers).
- **[DONE]** Export All parity (Windows exports **all frames + faces + slices** for a chosen mip in one action):
  - Qt provides: **Export All Frames**, **Export All Faces**, **Export All Slices**, **Export All Mipmaps**.
  - Qt also provides **Export All (Frames×Faces×Slices)** which prompts for the mip level.

## Clipboard

- **[DONE]** Copy image to clipboard (current view).
- **[DONE]** Paste image from clipboard to create a new VTF (`Edit → Paste Image as New VTF…`; Ctrl+V in the viewer).

## Viewer / Preview

- **[DONE]** Channel view (RGBA/RGB/R/G/B/A).
- **[DONE]** “Mask” toggle parity:
  - Qt “Mask” (Ctrl+W) now toggles **alpha compositing** of the current channel view over the selected background, matching Windows semantics (`VTFEdit/MainWindow.*`).
- **[DONE]** “Tile” toggle (Windows `View → Tile`, Ctrl+T).
- **[DONE]** Alpha background controls (Checker/Black/White/Custom color).
- **[DONE]** Zoom in/out/reset + fit‑to‑window.
- **[DONE]** Zoom to cursor (Ctrl+wheel) + panning (middle drag / Alt+drag).
- **[DONE]** Frame/Face/Slice/Mip selection.
- **[DONE]** Animated textures playback (basic).

## HDR / Float Preview

- **[DONE]** Exposure + tonemapped preview for float/HDR formats (ACES/Reinhard/Clamp) + cubemap preview support.
- **[DONE]** Windows parity mode:
  - Qt exposes a `VTFLib (Windows)` HDR preview mode that uses `VTFLib`’s FP16 exposure (`VTFLIB_FP16_HDR_EXPOSURE`) on the same **0.00–80.00** scale as Windows VTFEdit (default **20.00**) (`VTFEdit/MainWindow.*`, `legacy/VTFEdit-WinForms/VTFEdit.h`).
  - Qt still offers additional preview-only tonemap operators (ACES/Reinhard/Clamp) as an optional enhancement.

## VTF Properties / Editing

- **[DONE]** Edit VTF flags.
- **[DONE]** Edit minor version (VTF v7.x minor).
- **[DONE]** Start frame / bumpmap scale / reflectivity editing + “compute reflectivity”.
- **[DONE]** Generate mipmaps (selectable filter) and generate thumbnail.
- **[DONE]** Generate normal map (current/all frames) and generate sphere map.

## VTF Resources

- **[DONE]** Resource list/tree view.
- **[DONE]** View any resource as hex.
- **[DONE]** Export/import/replace/remove any resource as raw bytes.
- **[DONE]** Structured editors for common resources:
  - CRC
  - LOD settings
  - Texture settings (extended)
  - Key/Value data (KVD) viewer + VMT parsing display
- **[DONE]** Sheet resource decode + editor (v0/v1) + apply back into VTF.
- **[DONE]** “Information Resource” UI parity:
  - Qt now exposes an **Information** editor for the KVD resource (reads/writes the Windows `"Information"` schema) while still retaining a raw editor (`VTFEdit/VtfResourcesDialog.*`).

## VMT Editing / Workflow

- **[DONE]** VMT editor dock + save/save as.
- **[DONE]** Validate VMT (loose + strict) + optional live validation status.
- **[DONE]** VMT syntax highlighting (optional).
- **[DONE]** Find / Replace / Go to line.
- **[DONE]** “Open matching VMT” from a VTF (basic `$basetexture` heuristic) and “Open linked VTF” from a VMT.
- **[DONE]** “Create VMT File” parity:
  - Windows has a dedicated, feature-rich “Create VMT File” dialog (`legacy/VTFEdit-WinForms/VMTCreate.*`).
  - Qt mirrors the Windows dialog’s common texture slots/flags (including `$envmapmask`, `$detail`, `$normalmap`, `$dudvmap`, secondary base/bump, `%tooltexture`, `%keywords`, and the common boolean flags) (`VTFEdit/CreateVmtDialog.*`).

## Batch Convert

- **[DONE]** Convert folder (batch convert) workflow exists in Qt.
- **[DONE]** UI parity:
  - Qt batch convert now supports an optional **alpha format** selected per-image based on detected transparency, and can optionally create LOD clamp + Information (KVD) resources during conversion (`VTFEdit/BatchConvertDialog.*`, `VTFEdit/BatchConvertRunDialog.*`).

## App Options / Misc

- **[DONE]** Global “Options” dialog parity:
  - Qt has a unified options dialog (Tools → Options → Global Options…) and uses it as the source of global defaults/templates (`VTFEdit/OptionsDialog.*`).
  - Qt now mirrors the Windows format list (27 entries) and exposes “Texture Type” (Animated / Environment Map / Volume Texture) in both Global Options and the Create VTF Options dialog (`VTFEdit/OptionsDialog.*`, `VTFEdit/CreateVtfDialog.*`, `legacy/VTFEdit-WinForms/VTFOptions.h`).
- **[DONE]** “Warning Popups” toggle.
- **[DONE]** “Notification Sounds” toggle.
- **[DONE]** “Check For Updates”.
- **[DONE]** “Auto Create VMT File”.

## Deprecated / Broken in Windows (Not Targeted)

- **[DONE]** WAD convert:
  - Windows sources contain a WAD conversion dialog but it appears disabled/commented (“conversion is broken”).
  - Qt provides a working WAD2/WAD3 → VTF conversion workflow via **Tools → WAD Convert…** (`VTFEdit/WadConvertDialog.*`, `VTFEdit/WadFile.*`).

## Next Suggested Work (to reach “complete”)

No remaining `[PARTIAL]` / `[MISSING]` items in this checklist.
