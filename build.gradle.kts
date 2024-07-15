plugins {
    `maven-publish`
}

group = "casus.mala"
version = "0.0.01"
println(rootDir)
println(rootDir.resolve("liblammps"))
println(rootDir.resolve("liblammps/liblammps.so.0"))
publishing {
    publications {
        create<MavenPublication>("maven") {
            artifact(rootDir.resolve("liblammps/liblammps.so.0"))
        }
    }
    repositories {
        maven {
            name = "GitHubPackages"
            uri("https://maven.pkg.github.com/octocat/hello-world")
            credentials {
                username = System.getenv("GITHUB_ACTOR")
                password = System.getenv("GITHUB_TOKEN")
            }
        }
    }
}

