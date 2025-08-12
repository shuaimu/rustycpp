use clang::Entity;

#[derive(Debug, Clone)]
#[allow(dead_code)]
pub enum Annotation {
    Owns,
    Borrows(Option<String>),      // lifetime parameter
    MutBorrows(Option<String>),    // lifetime parameter
    Lifetime(String),               // lifetime name
}

#[allow(dead_code)]
pub fn extract_annotations(entity: &Entity) -> Vec<Annotation> {
    let mut annotations = Vec::new();
    
    // Look for C++ attributes like [[owns]], [[borrows]]
    if let Some(comment) = entity.get_comment() {
        // Parse annotations from comments for MVP
        // In future, we'd use actual C++ attributes
        if comment.contains("@owns") {
            annotations.push(Annotation::Owns);
        }
        if comment.contains("@borrows") {
            annotations.push(Annotation::Borrows(None));
        }
        if comment.contains("@mut_borrows") {
            annotations.push(Annotation::MutBorrows(None));
        }
    }
    
    annotations
}