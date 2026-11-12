// Starts `xe lsp` (the Xe language server) for Xe files: diagnostics while you type, formatting, go to
// definition and completion. The `xe` binary must be on PATH (the installed Xe, or `build/xe` of the repository).
use zed_extension_api as zed;

struct XeExtension;

impl zed::Extension for XeExtension {
    fn new() -> Self {
        XeExtension
    }

    fn language_server_command(
        &mut self,
        _language_server_id: &zed::LanguageServerId,
        worktree: &zed::Worktree,
    ) -> zed::Result<zed::Command> {
        let path = worktree
            .which("xe")
            .ok_or_else(|| "`xe` was not found on PATH; install Xe (see the README) first".to_string())?;
        Ok(zed::Command {
            command: path,
            args: vec!["lsp".to_string()],
            env: worktree.shell_env(),
        })
    }
}

zed::register_extension!(XeExtension);
