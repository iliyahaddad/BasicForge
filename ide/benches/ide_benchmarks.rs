// Benchmarks for BasicForge IDE
use criterion::{black_box, criterion_group, criterion_main, Criterion};
use basicforge_ide::language::LanguageServer;

fn bench_language_server(c: &mut Criterion) {
    let mut group = c.benchmark_group("language_server");
    
    let source = "Module Test\nSub Main()\nDim x As Integer = 10\nPrint(x)\nEnd Sub\nEnd Module";
    
    group.bench_function("analyze", |b| {
        b.iter(|| {
            let mut ls = LanguageServer::new();
            ls.analyze_file("test.bf", black_box(source));
        });
    });
    
    group.finish();
}

criterion_group!(benches, bench_language_server);
criterion_main!(benches);