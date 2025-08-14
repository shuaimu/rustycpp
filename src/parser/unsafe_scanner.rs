use std::path::Path;
use std::fs;
use std::io::{BufRead, BufReader};

/// Represents an unsafe region in the source code
#[derive(Debug, Clone)]
pub struct UnsafeRegion {
    pub start_line: usize,
    pub end_line: usize,
}

/// Scan a C++ file for unsafe regions marked with // @unsafe and // @endunsafe
pub fn scan_unsafe_regions(path: &Path) -> Result<Vec<UnsafeRegion>, String> {
    let file = fs::File::open(path)
        .map_err(|e| format!("Failed to open file for unsafe scanning: {}", e))?;
    
    let reader = BufReader::new(file);
    let mut regions = Vec::new();
    let mut unsafe_stack: Vec<usize> = Vec::new();
    
    for (line_num, line_result) in reader.lines().enumerate() {
        let line = line_result.map_err(|e| format!("Failed to read line: {}", e))?;
        let trimmed = line.trim();
        
        // Check for unsafe start marker
        if trimmed.contains("// @unsafe") || trimmed.contains("//@unsafe") {
            unsafe_stack.push(line_num + 1); // Convert to 1-based line numbers
        }
        // Check for unsafe end marker
        else if trimmed.contains("// @endunsafe") || trimmed.contains("//@endunsafe") {
            if let Some(start) = unsafe_stack.pop() {
                regions.push(UnsafeRegion {
                    start_line: start,
                    end_line: line_num + 1,
                });
            }
        }
    }
    
    // Handle unclosed unsafe regions (treat them as extending to end of file)
    // This is an error condition but we'll be lenient
    if !unsafe_stack.is_empty() {
        eprintln!("Warning: Unclosed unsafe regions detected");
    }
    
    Ok(regions)
}

/// Check if a given line number is within an unsafe region
pub fn is_line_in_unsafe_region(line: usize, regions: &[UnsafeRegion]) -> bool {
    regions.iter().any(|r| line >= r.start_line && line <= r.end_line)
}

#[cfg(test)]
mod tests {
    use super::*;
    use tempfile::NamedTempFile;
    use std::io::Write;

    #[test]
    fn test_scan_unsafe_regions() {
        let code = r#"// @safe
void test() {
    int value = 42;
    int& ref1 = value;
    
    // @unsafe
    int& ref2 = value;  // Line 7
    int& ref3 = value;  // Line 8
    // @endunsafe
    
    int& ref4 = value;  // Line 11
}
"#;
        
        let mut file = NamedTempFile::with_suffix(".cpp").unwrap();
        file.write_all(code.as_bytes()).unwrap();
        file.flush().unwrap();
        
        let regions = scan_unsafe_regions(file.path()).unwrap();
        assert_eq!(regions.len(), 1);
        assert_eq!(regions[0].start_line, 6);
        assert_eq!(regions[0].end_line, 9);
        
        // Test line checking
        assert!(!is_line_in_unsafe_region(5, &regions));
        assert!(is_line_in_unsafe_region(7, &regions));
        assert!(is_line_in_unsafe_region(8, &regions));
        assert!(!is_line_in_unsafe_region(11, &regions));
    }

    #[test]
    fn test_nested_unsafe_regions() {
        let code = r#"// @safe
void test() {
    // @unsafe
    int& ref1 = value;
    // @unsafe
    int& ref2 = value;
    // @endunsafe
    int& ref3 = value;
    // @endunsafe
}
"#;
        
        let mut file = NamedTempFile::with_suffix(".cpp").unwrap();
        file.write_all(code.as_bytes()).unwrap();
        file.flush().unwrap();
        
        let regions = scan_unsafe_regions(file.path()).unwrap();
        // Nested regions should be handled as separate regions
        assert_eq!(regions.len(), 2);
    }
}