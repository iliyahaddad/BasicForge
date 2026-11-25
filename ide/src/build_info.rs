// Build information for BasicForge IDE
// This file is generated at build time by build.rs using vergen

pub const VERSION: &str = env!("CARGO_PKG_VERSION");
pub const VERSION_MAJOR: u32 = 0;
pub const VERSION_MINOR: u32 = 1;
pub const VERSION_PATCH: u32 = 0;

fn git_commit() -> &'static str {
    option_env!("VERGEN_GIT_SHA").unwrap_or("unknown")
}

fn git_branch() -> &'static str {
    option_env!("VERGEN_GIT_BRANCH").unwrap_or("unknown")
}

fn git_tag() -> &'static str {
    option_env!("VERGEN_GIT_TAG").unwrap_or("")
}

fn build_date() -> &'static str {
    option_env!("VERGEN_BUILD_TIMESTAMP").unwrap_or("unknown")
}

fn build_host() -> &'static str {
    option_env!("VERGEN_CARGO_HOST_TRIPLE").unwrap_or("unknown")
}

fn rust_version() -> &'static str {
    option_env!("VERGEN_RUSTC_SEMVER").unwrap_or("unknown")
}

pub fn version_string() -> String {
    format!("{} ({})", VERSION, git_commit())
}

pub fn full_version_string() -> String {
    format!(
        "BasicForge IDE {}\n\
        Git commit: {}\n\
        Git branch: {}\n\
        Git tag: {}\n\
        Build date: {}\n\
        Build host: {}\n\
        Rust version: {}",
        VERSION, git_commit(), git_branch(), git_tag(), build_date(), build_host(), rust_version()
    )
}