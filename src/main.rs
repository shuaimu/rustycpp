use clap::Parser;
use colored::*;
use std::path::PathBuf;

mod parser;
mod ir;
mod analysis;
mod solver;
mod diagnostics;

#[derive(clap::Parser, Debug)]
#[command(name = "cpp-borrow-checker")]
#[command(about = "A static analyzer that enforces Rust-like borrow checking rules for C++")]
#[command(version)]
struct Args {
    /// C++ source file to analyze
    #[arg(value_name = "FILE")]
    input: PathBuf,

    /// Verbosity level
    #[arg(short, long, action = clap::ArgAction::Count)]
    verbose: u8,

    /// Output format (text, json)
    #[arg(long, default_value = "text")]
    format: String,
}

fn main() {
    let args = Args::parse();
    
    println!("{}", "C++ Borrow Checker".bold().blue());
    println!("Analyzing: {}", args.input.display());
    
    match analyze_file(&args.input) {
        Ok(results) => {
            if results.is_empty() {
                println!("{}", "✓ No borrow checking violations found!".green());
            } else {
                println!("{}", format!("✗ Found {} violation(s):", results.len()).red());
                for error in results {
                    println!("{}", error);
                }
                std::process::exit(1);
            }
        }
        Err(e) => {
            eprintln!("{}: {}", "Error".red().bold(), e);
            std::process::exit(1);
        }
    }
}

fn analyze_file(path: &PathBuf) -> Result<Vec<String>, String> {
    // Parse the C++ file
    let ast = parser::parse_cpp_file(path)?;
    
    // Build intermediate representation
    let ir = ir::build_ir(ast)?;
    
    // Perform borrow checking analysis
    let violations = analysis::check_borrows(ir)?;
    
    Ok(violations)
}
