import java.util.Map;
import java.util.LinkedHashMap;
import java.util.Arrays;
import java.util.Scanner;

public class main {

	static {
		System.loadLibrary("JniSearch");
	}

	public static void main(String[] args) {
		Map<String, String> parameters = new LinkedHashMap<>();

		for (String arg : args) {
		    String[] parts = arg.split("=", 2);
		    String key = parts[0];
		    if (key.startsWith("--")) {
			key = key.substring(2);
		    } else {
			key = key.substring(1);
		    }
		    String value = parts[1];

		    parameters.put(key, value);
		}

		Scanner input = new Scanner(System.in);
		if (args.length == 1) {
		    System.out.print("\033[H\033[J");
		    while(true) {
			System.out.print("> ");
			String query = input.nextLine();
			if (query.equals("!q")) {
			  break;
			}
			var result = JniSearch.search("config.json", parameters.get("index"), query);
			System.out.println(result);
		    }
		} else {
		    var result = JniSearch.search("config.json", parameters.get("index"), parameters.get("query"));
		    System.out.println(result);
		}

	}

}
