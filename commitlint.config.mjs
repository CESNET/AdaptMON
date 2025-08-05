export default {
    extends: ['@commitlint/config-conventional'],
    rules: {
        "fix-requires-reference": [2, "always"],
    },
    plugins: [
        {
            rules: {
                'fix-requires-reference': (parsed) => {
                    const { type, body } = parsed;

                    if (type !== 'fix') {
                        return [true];
                    }

                    const regex = /^Fixes:\s+[0-9a-f]{7,40}\s+\(".+"\)/m;

                    if (regex.test(body)) {
                        return [true];
                    }

                    return [
                        false,
                        'fix commits must reference a commit in the body using the format: Fixes: <hash> ("<description>")',
                    ];
                },
            }
        },
    ]
};
