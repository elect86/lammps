plugins {
    `maven-publish`
}

group = "casus.mala"
version = "0.0.01"

publishing {
    publications {
        create<MavenPublication>("maven") {
            artifact(rootDir.resolve("build/liblammps.so.0"))
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

