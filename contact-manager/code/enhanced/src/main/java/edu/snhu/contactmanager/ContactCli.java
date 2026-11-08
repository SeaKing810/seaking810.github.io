package edu.snhu.contactmanager;

import java.io.BufferedReader;
import java.io.EOFException;
import java.io.IOException;
import java.io.PrintWriter;

/**
 * Sean McKenna | CS 499 Milestone Two | November 7, 2026.
 * Presentation layer. Injected streams support end-to-end menu tests. Error
 * messages omit contact values and storage exception details to limit disclosure.
 */
public final class ContactCli {
    private final ContactService service;
    private final BufferedReader input;
    private final PrintWriter output;

    public ContactCli(ContactService service, BufferedReader input, PrintWriter output) {
        this.service = service;
        this.input = input;
        this.output = output;
    }

    public void run() throws IOException {
        output.println("Contact Manager | 1 Add | 2 Find | 3 List | 4 Edit | 5 Delete | 0 Exit");
        while (true) {
            try {
                String choice = read("Command: ").strip();
                switch (choice) {
                    case "0" -> { output.println("Goodbye."); return; }
                    case "1" -> {
                        Contact c = readContact(read("Contact ID: "));
                        service.add(c);
                        output.println("Contact saved.");
                    }
                    case "2" -> display(service.find(read("Contact ID: ")));
                    case "3" -> {
                        var contacts = service.list();
                        if (contacts.isEmpty()) output.println("No contacts saved.");
                        contacts.forEach(this::display);
                    }
                    case "4" -> {
                        String id = read("Contact ID to edit: ");
                        service.find(id);
                        output.println("Enter all replacement fields. ID stays unchanged.");
                        Contact c = readContact(id);
                        service.update(id, c.firstName(), c.lastName(), c.phone(), c.address());
                        output.println("Contact updated.");
                    }
                    case "5" -> {
                        String id = read("Contact ID to delete: ");
                        service.find(id);
                        if (read("Type YES to delete: ").equals("YES")) {
                            service.delete(id);
                            output.println("Contact deleted.");
                        } else output.println("Deletion cancelled.");
                    }
                    default -> output.println("Choose 0, 1, 2, 3, 4, or 5.");
                }
            } catch (EOFException ex) {
                output.println("Input ended; any incomplete command was cancelled.");
                return;
            } catch (IllegalArgumentException ex) {
                output.println("Not saved: " + ex.getMessage());
            } catch (IOException ex) {
                output.println("Operation failed; no in-memory change was applied. Check storage access.");
            } finally { output.flush(); }
        }
    }

    private Contact readContact(String id) throws IOException {
        return new Contact(id, read("First name (1-10): "), read("Last name (1-10): "),
            read("Phone (10 digits): "), read("Address (1-30): "));
    }

    private String read(String prompt) throws IOException {
        output.print(prompt);
        output.flush();
        String value;
        try { value = input.readLine(); }
        catch (IOException ex) {
            // A broken input stream cannot recover by redisplaying the menu.
            throw new EOFException("Input unavailable.");
        }
        if (value == null) throw new EOFException();
        return value;
    }

    private void display(Contact c) {
        output.printf("%s | %s %s | %s | %s%n", c.contactId(), c.firstName(), c.lastName(),
            c.phone(), c.address());
    }
}
