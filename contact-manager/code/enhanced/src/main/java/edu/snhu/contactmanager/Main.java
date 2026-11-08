package edu.snhu.contactmanager;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.InvalidPathException;
import java.nio.file.Path;

/**
 * Sean McKenna | CS 499 Milestone Two | November 7, 2026.
 * Composition root: connects the console, service, and local JSON repository.
 * Startup fails closed when existing data cannot be loaded safely.
 */
public final class Main {
    private Main() { }

    public static void main(String[] args) {
        if (args.length > 1) {
            System.err.println("Usage: Contact Manager [dedicated-data-directory]");
            System.exit(2);
        }
        try {
            Path directory = args.length == 1 ? Path.of(args[0]) : Path.of("contact-data");
            try (JsonFileContactRepository repository = new JsonFileContactRepository(directory)) {
                ContactService service = new ContactService(repository);
                new ContactCli(service, new BufferedReader(new InputStreamReader(System.in,
                    StandardCharsets.UTF_8)), new PrintWriter(System.out, true, StandardCharsets.UTF_8)).run();
            }
        } catch (IOException | InvalidPathException | SecurityException ex) {
            System.err.println("Unable to open or use the contact store. Close other instances and check "
                + "the directory, owner-only permissions, and JSON file. Existing data was not reset.");
            System.exit(1);
        }
    }
}
